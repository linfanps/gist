// 在页面里改写 document.title，并防止页面脚本把它改回去。
// Chrome 没有设置标签标题的 API，标签栏的文字就是 document.title，所以只能从页面这一侧动手。

let desiredTitle = null; // 当前希望显示的标题，null 表示不干预
let originalTitle = document.title; // 页面自己的标题，供 popup 显示
let sawOwnTitle = false; // 是否已经记录过页面自己的标题

function normalizeUrl(href) {
  try {
    const u = new URL(href);
    // 忽略 hash，避免锚点跳转产生一堆重复规则
    return u.origin + u.pathname + u.search;
  } catch {
    return href;
  }
}

function hostOf(href) {
  try {
    return new URL(href).hostname;
  } catch {
    return '';
  }
}

function matchRule(rules, href) {
  const url = normalizeUrl(href);
  const host = hostOf(href);
  // 精确页面规则优先于整站规则
  return (
    rules.find((r) => r.mode === 'url' && r.value === url) ||
    rules.find((r) => r.mode === 'host' && r.value === host) ||
    null
  );
}

function enforce() {
  if (desiredTitle !== null && document.title !== desiredTitle) {
    document.title = desiredTitle;
  }
}

// 监听 <title> 的变化。
// 为什么不劫持 Document.prototype 的 title setter：content script 跑在隔离世界，
// 和页面脚本共享 DOM 但不共享原型链，改 setter 拦不住页面自己的代码。观察 DOM 才可靠。
const observer = new MutationObserver(() => {
  if (desiredTitle === null) {
    // 没有规则时，持续跟踪页面真实标题
    originalTitle = document.title;
    sawOwnTitle = true;
  } else {
    enforce();
  }
});

function startObserving() {
  const root = document.documentElement;
  if (!root) return;
  observer.observe(root, { subtree: true, childList: true, characterData: true });
}

async function apply() {
  const { rules = [] } = await chrome.storage.sync.get('rules');
  const rule = matchRule(rules, location.href);

  if (!rule) {
    // 规则刚被删除时，把标题还原回页面自己的标题
    if (desiredTitle !== null && originalTitle) {
      desiredTitle = null;
      document.title = originalTitle;
    }
    desiredTitle = null;
    return;
  }

  // 第一次命中规则时，把当时的标题记下来当作"原标题"
  if (!sawOwnTitle && document.title) {
    originalTitle = document.title;
    sawOwnTitle = true;
  }
  desiredTitle = rule.title;
  enforce();
}

startObserving();
apply();

// document_start 时 <head> 可能还不存在，那次赋值可能不生效，所以在这两个时机补一次。
document.addEventListener('DOMContentLoaded', apply);
window.addEventListener('load', apply);

// 规则被 popup / options 改动后立即生效，不用刷新页面
chrome.storage.onChanged.addListener((changes, area) => {
  if (area === 'sync' && changes.rules) apply();
});

chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
  if (msg.type === 'getInfo') {
    sendResponse({
      ok: true,
      url: location.href,
      normalizedUrl: normalizeUrl(location.href),
      host: hostOf(location.href),
      originalTitle: originalTitle || document.title,
      currentTitle: document.title,
    });
    return; // 同步回复
  }

  if (msg.type === 'reapply') {
    // SPA 路由切换后由 background 触发
    sawOwnTitle = false;
    desiredTitle = null;
    apply();
    return;
  }

  if (msg.type === 'promptRename') {
    showOverlay();
    return;
  }
});

// 右键菜单触发的快速重命名浮层。
// 用 Shadow DOM 把样式和页面隔离开，避免被网站的 CSS 影响。
function showOverlay() {
  const existing = document.getElementById('__tab_renamer_overlay__');
  if (existing) existing.remove();

  const host = document.createElement('div');
  host.id = '__tab_renamer_overlay__';
  host.style.cssText = 'all:initial;position:fixed;inset:0;z-index:2147483647;';
  const shadow = host.attachShadow({ mode: 'closed' });

  shadow.innerHTML = `
    <style>
      .mask { position:fixed; inset:0; background:rgba(0,0,0,.35);
              display:flex; align-items:flex-start; justify-content:center; padding-top:18vh;
              font:14px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; }
      .box  { background:#fff; color:#111; border-radius:10px; padding:16px;
              width:min(420px,90vw); box-shadow:0 12px 40px rgba(0,0,0,.3); }
      .lbl  { font-size:12px; color:#666; margin-bottom:6px; }
      input { width:100%; box-sizing:border-box; padding:8px 10px; font-size:14px;
              border:1px solid #ccc; border-radius:6px; outline:none; }
      input:focus { border-color:#1a73e8; }
      .row  { display:flex; gap:8px; justify-content:flex-end; margin-top:12px; }
      button{ padding:6px 14px; font-size:13px; border-radius:6px; border:1px solid #ccc;
              background:#f5f5f5; cursor:pointer; }
      button.primary { background:#1a73e8; border-color:#1a73e8; color:#fff; }
      @media (prefers-color-scheme: dark) {
        .box { background:#2b2b2b; color:#eee; }
        .lbl { color:#aaa; }
        input { background:#1f1f1f; color:#eee; border-color:#555; }
        button { background:#3a3a3a; border-color:#555; color:#eee; }
      }
    </style>
    <div class="mask">
      <div class="box">
        <div class="lbl">标签名称（留空则恢复原标题，仅对当前页面生效）</div>
        <input id="i" type="text" />
        <div class="row">
          <button id="cancel">取消</button>
          <button id="ok" class="primary">保存</button>
        </div>
      </div>
    </div>
  `;

  document.documentElement.appendChild(host);

  const input = shadow.getElementById('i');
  input.value = desiredTitle ?? document.title;
  input.focus();
  input.select();

  const close = () => host.remove();

  const save = async () => {
    const value = input.value.trim();
    const { rules = [] } = await chrome.storage.sync.get('rules');
    const url = normalizeUrl(location.href);
    const next = rules.filter((r) => !(r.mode === 'url' && r.value === url));
    if (value) {
      next.push({
        id: `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
        mode: 'url',
        value: url,
        title: value,
      });
    } else if (desiredTitle !== null) {
      // 清空时恢复原标题
      document.title = originalTitle;
    }
    await chrome.storage.sync.set({ rules: next });
    close();
  };

  shadow.getElementById('cancel').addEventListener('click', close);
  shadow.getElementById('ok').addEventListener('click', save);
  input.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') save();
    if (e.key === 'Escape') close();
  });
}
