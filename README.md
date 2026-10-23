# gist

个人配置、规则和小工具的归档仓库。各个顶层目录相互独立，可按需单独使用；带有历史环境信息的配置应先阅读、再复制到本机。

## 目录定位

| 目录 | 作用 | 定位 |
| --- | --- | --- |
| [`conf/`](./conf/) | 各 AI 编码工具共用的全局工作规定 | `conf/AGENTS.md` 为单一来源；本机 `~/.AGENTS.md` 以及 Claude Code、Codex、Trae 的配置文件均软链至此 |
| [`shadowrocket-rules/`](./shadowrocket-rules/) | Shadowrocket 配置与独立规则集 | 当前维护的网络规则；公开文件不包含 MITM CA 私钥和口令 |
| [`tab-renamer/`](./tab-renamer/) | 自定义 Chrome 标签页标题的 Manifest V3 扩展 | 可直接以“加载已解压的扩展程序”方式安装，详细说明见目录内 README |
| [`vim/`](./vim/) | 从 `linfanps/vim` 迁入的 Vim 配置、脚本和模板 | 旧版 Vim 开发环境归档，以 Vundle 管理插件，适合参考或在旧环境中复用 |

## AI 工具配置

`conf/AGENTS.md` 是给 AI 编码工具读的全局工作规定，涵盖破坏性操作的确认边界、浏览器自动化约定和服务器登录方式。它是唯一的源文件，三端通过软链共用：

```bash
ln -s ~/Code/ccproject/Assistant/gist/conf/AGENTS.md ~/.AGENTS.md
ln -s ~/.AGENTS.md ~/.claude/CLAUDE.md    # Claude Code
ln -s ~/.AGENTS.md ~/.codex/AGENTS.md     # Codex
ln -s ~/.AGENTS.md ~/.trae/AGENTS.md      # Trae
```

文件里的服务器只写 `~/.ssh/config` 中的别名（如 `ssh TencentCloudHost`），真实地址和用户名不进仓库。使用前需要在本机 `~/.ssh/config` 里配好对应的 `Host` 条目。

## Shadowrocket 规则

`shadowrocket-rules/CRYPTO.list` 是独立的加密货币服务分流规则集，可在 Shadowrocket 配置的 `[Rule]` 中引用：

```ini
RULE-SET,https://raw.githubusercontent.com/linfanps/gist/master/shadowrocket-rules/CRYPTO.list,Crypto节点
```

`shadowrocket-rules/last.conf` 是定制后的完整配置，原始地址为：

```text
https://raw.githubusercontent.com/linfanps/gist/master/shadowrocket-rules/last.conf
```

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
