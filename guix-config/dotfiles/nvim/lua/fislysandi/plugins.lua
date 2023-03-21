-- plugin.lua

return require('packer').startup(function()
  -- add your plugins here
  use 'tpope/vim-fugitive'
  use 'nvim-telescope/telescope.nvim'
  use 'justinmk/vim-sneak'
  use 'itchyny/lightline.vim'
  use 'vijaymarupudi/nvim-fzf'
  use 'kylechui/nvim-surround'
  use 'tpope/vim-commentary'
  use 'mg979/vim-visual-multi' 
  use 'hrsh7th/nvim-cmp'
  use('nvim-treesitter/nvim-treesitter', {run = ':TSUpdate'})
  use 'nvim-lua/plenary.nvim'
  use 'mbbill/undotree'
  use 'nvim-treesitter/playground'
  use 'theprimeagen/harpoon'
  use {
  'VonHeikemen/lsp-zero.nvim',
  branch = 'v1.x',
  requires = {
    -- LSP Support
    {'neovim/nvim-lspconfig'},             -- Required
    {'williamboman/mason.nvim'},           -- Optional
    {'williamboman/mason-lspconfig.nvim'}, -- Optional

    -- Autocompletion
    {'hrsh7th/nvim-cmp'},         -- Required
    {'hrsh7th/cmp-nvim-lsp'},     -- Required
    {'hrsh7th/cmp-buffer'},       -- Optional
    {'hrsh7th/cmp-path'},         -- Optional
    {'saadparwaiz1/cmp_luasnip'}, -- Optional
    {'hrsh7th/cmp-nvim-lua'},     -- Optional

    -- Snippets
    {'L3MON4D3/LuaSnip'},             -- Required
    {'rafamadriz/friendly-snippets'}, -- Optional
  }
}
end)

