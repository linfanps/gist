vim config
===========

Bundle requires Vim version 7.3.584+
YouCompleteMe requires python2 support

++++++++++++++
git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim

vim +PluginInstall +qall

cd ~/.vim/bundle/YouCompleteMe
python install.py  --clang-completer --gocode-completer

===================
yld: warning, LC_RPATH @executable_path/../lib in /Users/linfan/.vim/bundle/YouCompleteMe/third_party/ycmd/libclang.dylib being ignored in restricted program because of @executable_path?
Solution:
cd to the YouCompleteMe installation directory (e.g. ~/.vim/bundle/YouCompleteMe)
cd third_party/ycmd
install_name_tool -delete_rpath '@executable_path/../lib' libclang.dylib

