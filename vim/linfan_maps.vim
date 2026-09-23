" Press Insert to save and quit
imap <INS> <ESC>:wq<CR>
nmap <INS> :wq<CR>

" Press Del to quit without save
imap <DEL> <ESC>:q!<CR>
nmap <DEL> :q!<CR>

" Press F11 F12 to jump
nmap <F11> <C-]>
nmap <F12> <C-t>

" Press F10 to write
imap <F10> <ESC>:w<CR>
nmap <F10> :w<CR>"

imap <F9> <ESC>:wq<CR>
nmap <F9> :wq<CR>

imap <F8> <ESC>:q!<CR>
nmap <F8> :q!<CR>

" Press F6 F7 to move to next tab
map <F6> <C-TAB>
map <F7> <C-S-TAB>

"For HHKB keyboard need to Press Fn+8 to F8
nmap 7 :w<CR>
nmap 8 :q!<CR>
nmap 9 :wq<CR>


" auto complete () {} [] <> '' ""
inoremap ( ()<ESC>i
inoremap [ []<ESC>i
""inoremap < <><ESC>i
inoremap << <<<ESC>a
inoremap ' ''<ESC>i
inoremap " ""<ESC>i
au FileType c,cpp,go,php,css inoremap { {<CR>}<ESC>O
au FileType ruby,python inoremap { {}<ESC>i
au FileType html inoremap < <><ESC>i


nnoremap tree :NERDTreeToggle<CR>
nnoremap TL :TlistToggle<CR>
nnoremap MBE :TMiniBufExplorer<CR>

"Cscope  <c-R><C-W>获得光标处单词
nnoremap scs :cs find s  <C-R><C-W><CR>
nnoremap scg :cs find g  <C-R><C-W><CR>
nnoremap sce :cs find e  <C-R><C-W><CR>
""nmap <C-,>t :cs find t <C-R><C-W><CR><CR>
""nmap <C-,>e :cs find e <C-R><C-W><CR><CR>
""nmap <C-,>f :cs find f <C-R>=expand("<cfile>")<CR><CR>
""nmap <C-,>i :cs find i ^<C-R>=expand("<cfile>")<CR>$<CR>
""nmap <C-,>d :cs find d <C-R><C-W><CR><CR>


"fuzzyfinder
map ff :FufFile<CR>
map fb :FufBuffer<CR>
