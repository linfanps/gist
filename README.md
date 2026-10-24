# gist

个人配置、规则和小工具的归档仓库。各个顶层目录相互独立，可按需单独使用；带有历史环境信息的配置应先阅读、再复制到本机。

## 目录定位

| 目录 | 作用 | 定位 |
| --- | --- | --- |
| [`conf/`](./conf/) | 本机常用配置：AI 工具指令、shell、tmux、窗口管理 | 每个文件都是单一来源，本机对应的点文件软链至此；密钥、SSH 和公司相关配置一律不入库 |
| [`shadowrocket-rules/`](./shadowrocket-rules/) | Shadowrocket 入口配置、自维护规则集与上游规则快照 | 手机订阅 `main.conf`，规则本体全部以 `RULE-SET` 引用；公开文件不包含 MITM CA 私钥和口令 |
| [`tab-renamer/`](./tab-renamer/) | 自定义 Chrome 标签页标题的 Manifest V3 扩展 | 可直接以“加载已解压的扩展程序”方式安装，详细说明见目录内 README |
| [`vim/`](./vim/) | 从 `linfanps/vim` 迁入的 Vim 配置、脚本和模板 | 旧版 Vim 开发环境归档，以 Vundle 管理插件，适合参考或在旧环境中复用 |

## 本机配置（conf/）

`conf/` 收集本机的常用配置文件，每个都是单一来源，由家目录下对应的点文件软链过来：

| 仓库文件 | 软链到 | 说明 |
| --- | --- | --- |
| `conf/AGENTS.md` | `~/.AGENTS.md` | 给 AI 编码工具读的全局工作规定；`~/.claude/CLAUDE.md`、`~/.codex/AGENTS.md`、`~/.trae/AGENTS.md` 再软链到 `~/.AGENTS.md`，三端共用一份 |
| `conf/zshrc` | `~/.zshrc` | zsh 配置；文件末尾附有迁移到新机器时的完整安装步骤 |
| `conf/tmux.conf` | `~/.tmux.conf` | tmux 配置，prefix 为 `C-q`；插件由 tpm 管理 |
| `conf/slate` | `~/.slate` | Slate 窗口管理的快捷键与布局 |

软链方式：

```bash
ln -s "$PWD/conf/zshrc"     ~/.zshrc
ln -s "$PWD/conf/tmux.conf" ~/.tmux.conf
ln -s "$PWD/conf/slate"     ~/.slate
ln -s "$PWD/conf/AGENTS.md" ~/.AGENTS.md
ln -s ~/.AGENTS.md ~/.claude/CLAUDE.md
ln -s ~/.AGENTS.md ~/.codex/AGENTS.md
ln -s ~/.AGENTS.md ~/.trae/AGENTS.md
```

以下内容**不**纳入本仓库，迁移时需在新机器上单独准备：

- `~/.KEY` — API key 等凭据，由 `conf/zshrc` 条件加载（文件不存在时静默跳过）
- `~/.ssh/config`、`~/.ssh/id_*` — 服务器别名、真实地址和私钥。`conf/` 中的文件只写别名（如 `ssh TencentCloudHost`），不写 IP 和用户名
- `~/.gitconfig-work` — 公司身份与内网仓库配置
- `~/.tmux/plugins/`、`~/.tmux/resurrect/` — 插件由 tpm 克隆；会话存档含 pane 屏幕内容，不应外传

## Shadowrocket 规则

`main.conf` 是手机订阅的入口。它只保留 `[General]`、节点组和策略决定，规则本体全部通过 `RULE-SET` 引用：

```text
https://raw.githubusercontent.com/linfanps/gist/master/shadowrocket-rules/main.conf
```

| 文件 | 内容 | 维护方式 |
| --- | --- | --- |
| `main.conf` | 入口配置：`[General]`、`CRYPTO` 节点组、规则引用与兜底 | 手工 |
| `CRYPTO.list` | 交易所与 DeFi 域名 | 手工 |
| `reference/TOP500_REJECT.list` | 广告与追踪域名 | 脚本生成 |
| `reference/TOP500_PROXY.list` | GEOIP 判为 CN 但需走代理的域名与网段 | 脚本生成 |

`reference/` 下的文件由 [Shadowrocket-ADBlock-Rules-Forever](https://github.com/Johnshall/Shadowrocket-ADBlock-Rules-Forever) 的完整配置切分而来，不要手工编辑。更新上游名单：

```bash
cd shadowrocket-rules && ./update-upstream.sh   # 只写文件，不碰 git
```

脚本在下载失败、上游出现未知策略名或抽取条数塌陷时会直接退出，而不是生成错误的规则。跑完提交推送，手机下次拉 `RULE-SET` 才会看到新名单 —— 没有自动更新。

`[Rule]` 是自上而下首个匹配生效，改 `main.conf` 前需要知道这几个顺序约束：

- `TOP500_REJECT.list` 必须在 `TOP500_PROXY.list` 之前 —— 后者含 `linkedin.cn`，前者含 358 个 `.cn` 广告域名
- `DOMAIN-SUFFIX,cn,DIRECT` 必须在两者之后，否则会把 `linkedin.cn` 和那 358 个域名抢成直连
- 个人规则放最前，`FINAL,PROXY` 放最后

`CRYPTO` 节点组用 `policy-regex-filter=CRYPTO` 按节点名筛选，配置里不写死任何节点地址。

上游的 Direct 块（境外可直连站点）未纳入，这类流量由 `GEOIP,CN,DIRECT` 和 `FINAL,PROXY` 决定：代价是多走一些代理流量，换取规则文件数量和维护成本都更低。

MITM 的 `ca-p12` 和 `ca-passphrase` 不应提交到公开仓库。需要 HTTPS 解密时，应在 Shadowrocket 本地生成证书，或保存在仅本机/iCloud 私有同步的证书模块中。

## Tab Renamer

`tab-renamer/` 是一个无构建步骤的 Chrome 扩展，用于按页面 URL 或网站域名覆盖 `document.title`。规则保存在 `chrome.storage.sync`，支持导入、导出与跨设备同步。安装、使用和文件说明见 [`tab-renamer/README.md`](./tab-renamer/README.md)。

## Vim 配置

`vim/` 来自 [`linfanps/vim`](https://github.com/linfanps/vim) 的 `ea52f39` 版本，主要内容如下：

| 路径 | 作用 |
| --- | --- |
| `vim/vimrc` | 编辑器主配置，包括编码、缩进、搜索、模板和插件入口 |
| `vim/plugins.vim` | Vundle 插件声明及 YouCompleteMe 相关设置 |
| `vim/linfan_maps.vim` | 保存、退出、跳转、补全和 Cscope 等快捷键 |
| `vim/autoload/` | FuzzyFinder 与 L9 的自动加载脚本 |
| `vim/plugin/` | FuzzyFinder、L9 和 MiniBufExplorer 等插件入口 |
| `vim/colors/` | Molokai 配色 |
| `vim/bin/` | 从编译参数生成 `.clang_complete` 的辅助脚本 |
| `vim/tools/template/` | 新建 C、C++、Go、PHP、Python 文件时使用的模板 |
| `vim/bundle/Vundle.vim/` | 仓库内附带的 Vundle 插件管理器 |

传统安装方式：

```bash
cp -R vim ~/.vim
ln -s ~/.vim/vimrc ~/.vimrc
vim +PluginInstall +qall
```

这套配置面向旧版 Vim，并保留了依赖 Python 2 的旧 YouCompleteMe 安装说明；在现代 Vim、Neovim 或新版插件环境中使用前需要自行调整。

迁移时没有纳入 Vim 自动生成的 `.netrwhist`，因为它只记录机器本地目录历史。源仓库中的 YouCompleteMe、ack.vim、closetag.vim 和 vim-go 是未配置 `.gitmodules` 的 Git 链接，不含实际插件源码；这些依赖继续由 `plugins.vim` 中的 Vundle 声明负责安装。
