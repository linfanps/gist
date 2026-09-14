// Service worker：注册右键菜单，并在 SPA 路由变化时通知 content script 重新匹配规则。

const MENU_ID = 'tab-renamer-rename';

chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.removeAll(() => {
    chrome.contextMenus.create({
      id: MENU_ID,
      title: '重命名此标签',
      contexts: ['page'],
    });
  });
});

chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId !== MENU_ID || !tab?.id) return;
  send(tab.id, { type: 'promptRename' });
});

// changeInfo.url 在 SPA 用 history.pushState 换路由时也会触发，
// 正好用来让 content script 重新查一次规则。
chrome.tabs.onUpdated.addListener((tabId, changeInfo) => {
  if (changeInfo.url) send(tabId, { type: 'reapply' });
});

// content script 不存在时（chrome:// 页面、扩展页面等）sendMessage 会 reject，忽略即可。
function send(tabId, msg) {
  chrome.tabs.sendMessage(tabId, msg).catch(() => {});
}
