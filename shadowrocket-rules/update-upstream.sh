#!/usr/bin/env bash
#
# 从上游拉取 top500 白名单 + 广告过滤规则，切成 Shadowrocket RULE-SET 能引用的 .list。
#
#   用法: ./update-upstream.sh
#
# 只写 reference/ 下的文件，不做 git add / commit / push。
# 生成完自己看一眼 git diff 再决定提交。
#
# 上游 .conf 把规则按策略分成连续几块，本脚本只取其中 Reject 和 Proxy 两块：
# Direct 块（境外可直连站点）已按方案 B 舍弃，交给 main.conf 里的 GEOIP,CN,DIRECT 兜。

set -euo pipefail

UPSTREAM='https://johnshall.github.io/Shadowrocket-ADBlock-Rules-Forever/sr_top500_whitelist_ad.conf'
RAWBASE='https://raw.githubusercontent.com/linfanps/gist/master/shadowrocket-rules/reference'

# 上游策略块的已知顺序。变了不影响本方案（只用 Reject / Proxy），但会提示一声。
BASELINE='Reject Direct Proxy Direct DIRECT PROXY '

HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/reference"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

die() { printf '错误：%s\n' "$1" >&2; exit 1; }

# ---------- 1. 下载 ----------
printf '拉取 %s\n' "$UPSTREAM"
curl -fsSL --max-time 180 -o "$TMP/raw" "$UPSTREAM" \
  || die "下载失败。raw/github 被阻断时会这样，挂代理或换网络再试。"

size=$(wc -c < "$TMP/raw" | tr -d ' ')
[ "$size" -gt 1000000 ] \
  || die "上游只有 $size 字节（预期 >1MB），可能拿到的是错误页而不是配置，已中止。"

tr -d '\r' < "$TMP/raw" > "$TMP/u"
build_time=$(sed -n 's/^# build time: *//p' "$TMP/u" | head -1)
printf '  %s 字节，上游构建时间 %s\n' "$size" "${build_time:-未知}"

# ---------- 2. 只保留 [Rule] 段 ----------
awk '/^\[Rule\]/{f=1;next} /^\[/{f=0} f' "$TMP/u" > "$TMP/rules"

# ---------- 3. 断言：没有本脚本不认识的策略 ----------
# 出现新策略说明上游改了结构，此时静默跳过那些规则会导致路由悄悄变样，宁可直接失败。
awk -F, 'NF>1 && /^[A-Z]/{print $NF}' "$TMP/rules" | sort -u > "$TMP/policies"
if grep -vxE 'Reject|Direct|Proxy|DIRECT|PROXY' "$TMP/policies" > "$TMP/unknown"; then
  printf '错误：上游出现未知策略，这些规则会被漏掉：\n' >&2
  sed 's/^/  /' "$TMP/unknown" >&2
  printf '请先确认它们该归到哪个 .list，再更新本脚本的白名单。\n' >&2
  exit 1
fi

# ---------- 4. 提示：策略块顺序是否变化 ----------
shape=$(awk -F, 'NF>1 && /^[A-Z]/{p=$NF; if(p!=l){printf "%s ",p; l=p}}' "$TMP/rules")
if [ "$shape" != "$BASELINE" ]; then
  printf '提示：上游策略块顺序现在是「%s」，基线是「%s」。\n' "$shape" "$BASELINE"
  printf '      本方案只用 Reject 和 Proxy 两块，顺序变化不改变结果，但值得看一眼。\n'
fi

# ---------- 5. 抽取：按策略筛行，去掉策略后缀和注释 ----------
extract() {
  awk -F, -v p="$1" 'NF>1 && /^[A-Z]/ && $NF==p { sub(/,[^,]*$/, ""); print }' "$TMP/rules"
}
extract Reject > "$TMP/reject"
extract Proxy  > "$TMP/proxy"

reject_n=$(wc -l < "$TMP/reject" | tr -d ' ')
proxy_n=$(wc -l < "$TMP/proxy" | tr -d ' ')

# 条数塌陷通常意味着上游换了策略名的大小写写法，同样直接失败。
[ "$reject_n" -ge 40000 ] || die "reject 只抽到 $reject_n 条（预期 >=40000），已中止。"
[ "$proxy_n"  -ge 200 ]   || die "proxy 只抽到 $proxy_n 条（预期 >=200），已中止。"

# ---------- 6. 写出 ----------
mkdir -p "$OUT"
emit() {
  local src="$TMP/$1" name="$2" policy="$3" note="$4"
  local dst="$OUT/$name"
  local n old
  n=$(wc -l < "$src" | tr -d ' ')
  old=$([ -f "$dst" ] && grep -cvE '^#|^[[:space:]]*$' "$dst" || echo 0)
  {
    printf '# 本文件由 update-upstream.sh 生成，不要手工编辑\n'
    printf '# 来源: %s\n' "$UPSTREAM"
    printf '# 上游构建时间: %s\n' "${build_time:-未知}"
    printf '# 生成时间: %s\n' "$(date -u '+%Y-%m-%d %H:%M:%SZ')"
    printf '# 条数: %s\n' "$n"
    printf '# 用法: RULE-SET,%s/%s,%s\n' "$RAWBASE" "$name" "$policy"
    printf '# 说明: %s\n\n' "$note"
    cat "$src"
  } > "$dst"
  printf '  %-20s %6s 条（原 %s）\n' "$name" "$n" "$old"
}

printf '写入 %s\n' "$OUT"
emit reject TOP500_REJECT.list REJECT '广告与追踪域名，上游 adblock 名单'
emit proxy  TOP500_PROXY.list  PROXY  'GEOIP 判为 CN 但必须走代理的域名与网段（地区检测、定位等）'

printf '\n完成。接下来：\n'
printf '  git -C %s diff --stat\n' "$(cd "$HERE/.." && pwd)"
printf '  确认无误后再 git add / commit / push\n'
