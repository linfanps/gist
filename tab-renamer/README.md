# Tab Renamer

自定义 Chrome 标签页名称。竖排标签栏、标签搜索、窗口标题都会同步生效。

## 安装

1. 打开 `chrome://extensions`
2. 右上角打开**开发者模式**
3. 点**加载已解压的扩展程序**，选中本目录
4. 点地址栏右侧的拼图图标，把 Tab Renamer **固定**到工具栏

## 使用

- **重命名**：在目标页面点扩展图标 → 输入名称 → 选「仅此页面」或「整个网站」→ 保存
- **快捷方式**：页面上右键 → 「重命名此标签」，弹出浮层直接改（只作用于当前页面）
- **恢复**：popup 里点「恢复原标题」，会删掉命中当前页面的所有规则并刷新
- **批量管理**：popup 底部「管理全部规则」，可直接编辑标题或删除；标题清空等同删除
- **备份 / 迁移**：规则页顶部的「导出」下载 JSON，「导入」按 `(范围, 匹配目标)` 合并，
  重复的以导入的为准；想整体还原就先「清空全部」再导入

规则存在 `chrome.storage.sync`，登录同一个 Google 账号的设备之间会自动同步。

## 原理

Chrome 没有提供设置标签标题的 API——标签栏显示的就是页面的 `document.title`。
所以扩展的做法是在页面里改写 `document.title`：

- content script 在 `document_start` 注入，按 URL 查规则并赋值
- 用 `MutationObserver` 盯住 `<title>`，页面脚本改回去就再改一次
  （不能劫持 `Document.prototype` 的 title setter：content script 在隔离世界，
  与页面脚本共享 DOM 但不共享原型链，拦不住页面自己的代码）
- SPA 用 `history.pushState` 换路由时，由 background 监听 `chrome.tabs.onUpdated`
  的 `changeInfo.url` 通知 content script 重新匹配

规则结构（存在 `storage.sync` 的 `rules` 数组里）：

```js
{
  id:    string,          // 唯一 id
  mode:  "url" | "host",  // 精确页面 / 整个网站
  value: string,          // mode=url 时为去掉 hash 的 URL；mode=host 时为域名
  title: string           // 自定义标题
}
```

精确页面规则优先于整站规则。

## 限制

- `chrome://` 页面、扩展页面、新标签页、内置 PDF 阅读器**改不了**——Chrome 禁止在这些页面注入 content script
- 只改文字，**不改网页图标**。如果也想换图标，需要额外替换页面里的 `<link rel="icon">`
- 规则按 URL 匹配，同一个 URL 开两个标签会显示同一个名字
- 未打包扩展的 ID 由**文件夹绝对路径**哈希得出。移动或改名本目录，ID 会变，
  `chrome.storage.sync` 里的规则（按扩展 ID 隔离）就读不到了。
  换位置前先用规则页的「导出」存一份 JSON，装好后再「导入」回来

## 文件

| 文件 | 作用 |
| --- | --- |
| `manifest.json` | MV3 清单 |
| `content.js` | 改写并守住 `document.title`；右键重命名浮层 |
| `background.js` | 右键菜单注册；SPA 路由变化时通知 content script |
| `popup.html/js` | 点图标弹出的重命名面板 |
| `options.html/js` | 规则列表管理 |
| `icons/` | 扩展图标（16/32/48/128）及生成脚本 |

## 图标

蓝底（`#1a73e8`，和扩展 UI 主色一致）+ 白色标签页 + 铅笔。

图标由 `icons/gen_icon.py` 生成，纯标准库：自己做超采样光栅化再用 zlib 手写 PNG，
不依赖 Pillow / ImageMagick / SVG 光栅化器。改完形状参数后重跑即可：

```bash
python3 icons/gen_icon.py
```

几何都定义在 0..1 的单位正方形里（`shade()` 函数），改配色只需动 `BLUE` / `WHITE`。
