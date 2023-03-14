-- plugin.lua

return require('packer').startup(function()
  -- add your plugins here
  use 'tpope/vim-fugitive'
  use 'nvim-telescope/telescope.nvim'
  use 'neovim/nvim-lspconfig'
  use 'justinmk/vim-sneak'
  use 'itchyny/lightline.vim'
  use 'vijaymarupudi/nvim-fzf'
  use 'kylechui/nvim-surround'
  use 'tpope/vim-commentary'
  use 'mg979/vim-visual-multi' 
  use 'hrsh7th/nvim-cmp'
  use 'nvim-treesitter/nvim-treesitter'
  use 'nvim-lua/plenary.nvim'
end)

