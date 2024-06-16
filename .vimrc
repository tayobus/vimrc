" 기본 설정
set nocompatible            " Vi 호환 모드를 비활성화 (Vim의 고급 기능 사용)
filetype off                " 파일 타입 감지를 끄고 설정을 시작

" 플러그인 관리 (예: vim-plug)
call plug#begin('~/.vim/plugged')

" 예제 플러그인
Plug 'preservim/nerdtree'
Plug 'kien/ctrlp.vim'
Plug 'vim-airline/vim-airline'
Plug 'junegunn/fzf'
Plug 'junegunn/fzf.vim'
Plug 'tpope/vim-surround'
Plug 'tpope/vim-commentary'
Plug 'vim-syntastic/syntastic'
Plug 'ycm-core/YouCompleteMe'
Plug 'tpope/vim-fugitive'

call plug#end()             " 플러그인 설정 종료

" 플러그인 설정
" NERDTree 설정
map <C-n> :NERDTreeToggle<CR>  " Ctrl+n으로 NERDTree 토글

" ctrlp 설정
let g:ctrlp_map = '<C-p>'       " Ctrl+p로 CtrlP 실행
let g:ctrlp_cmd = 'CtrlP'

" vim-airline 설정
let g:airline#extensions#tabline#enabled = 1

" fzf 설정
let g:fzf_command_prefix = 'Fzf'

" vim-surround 기본 설정은 필요 없음

" vim-commentary 기본 설정은 필요 없음

" syntastic 설정
let g:syntastic_always_populate_loc_list = 1
let g:syntastic_auto_loc_list = 1
let g:syntastic_check_on_open = 1
let g:syntastic_check_on_wq = 0

" YouCompleteMe 설정
let g:ycm_global_ycm_extra_conf = '~/.vim/.ycm_extra_conf.py'
let g:ycm_confirm_extra_conf = 0

" vim-fugitive 기본 설정은 필요 없음

" 일반 설정
syntax on                   " 구문 강조를 켭니다
filetype plugin indent on   " 파일 타입 감지, 플러그인 및 자동 들여쓰기를 켭니다

" 사용자 인터페이스 설정
set number                  " 행 번호를 표시합니다
set relativenumber          " 상대 행 번호를 표시합니다
set cursorline              " 커서가 있는 행을 강조합니다
set showcmd                 " 입력 중인 명령을 표시합니다
set showmode                " 현재 모드를 표시합니다
set wildmenu                " 명령 입력 시 탭 완성 기능을 강화합니다
set lazyredraw              " 매크로 실행 및 스크롤 중 화면 갱신을 지연합니다

" 편집 설정
set tabstop=4               " 탭 문자의 폭을 설정합니다
set shiftwidth=4            " 들여쓰기에 사용되는 스페이스 수를 설정합니다
set expandtab               " 탭을 스페이스로 변환합니다
set autoindent              " 자동 들여쓰기를 켭니다
set smartindent             " 스마트 들여쓰기를 켭니다
set clipboard=unnamedplus   " 시스템 클립보드를 사용합니다

" 검색 설정
set ignorecase              " 대소문자를 무시한 검색을 합니다
set smartcase               " 대소문자가 섞인 경우 대소문자를 구분하여 검색합니다
set incsearch               " 검색어를 입력하는 동안 실시간으로 검색합니다
set hlsearch                " 검색 결과를 강조합니다

" 스왑 파일 설정
set swapfile                " 스왑 파일을 생성합니다
set directory=~/.vim/swap//     " 스왑 파일을 저장할 디렉토리 설정

" 기타 설정
set splitright              " 수직 분할 시 새로운 창을 오른쪽에 엽니다
set splitbelow              " 수평 분할 시 새로운 창을 아래에 엽니다
set hidden                  " 수정된 파일을 저장하지 않고 다른 파일을 열 수 있게 합니다
set mouse=a                 " 마우스 지원을 활성화합니다

" 명령어 단축키 설정
nnoremap <C-s> :w<CR>       " Ctrl+s로 파일 저장
inoremap <C-s> <Esc>:w<CR>a " Ctrl+s로 파일 저장 (삽입 모드)

" 색상 설정
set background=dark         " 배경 색상을 어둡게 설정
colorscheme desert          " 색상 테마 설정

