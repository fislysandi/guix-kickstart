-- load packer
require('packer').init()

-- source packer configuration
require('fislysandi.packer')

-- source plugin definitions
require('fislysandi.plugins')

require('fislysandi.keymaps')


-- Enable syntax highlighting

require("nvim-treesitter.install").prefer_git = true


local options = {
  number = true,
  relativenumber = true,
  autoindent = true,
  tabstop = 4,
  shiftwidth = 4,
  smarttab = true,
  softtabstop = 4,
  mouse = "a",
  langmenu = 'en_US',
}

vim.cmd("syntax on")

--vim.api.nvim_set_keymap('n', 'Y', '"+y', { noremap = true })

-- if you use windows uncomment this
--vim.o.shell = "pwsh"



-- Set the Vim options
for k, v in pairs(options) do
  vim.opt[k] = v
end

--.guile-repl.socket
vim.g["conjure#filetype#scheme"] = "conjure.client.guile.socket"
vim.g["conjure#client#guile#socket#pipename"] = "guile --listen=/home/fislysandi/Desktop/dev/git/guix-kickstart/.guile-repl.socket"

-- Set the runtime path
--vim.o.runtimepath = vim.o.runtimepath .. ';' .. 'C:/Users/angryteapodthebonedo/AppData/Local/nvim'


