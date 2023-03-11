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


vim.api.nvim_set_keymap('n', 'Y', '"+y', { noremap = true })


vim.o.shell = "pwsh"



-- Set the Vim options
for k, v in pairs(options) do
  vim.opt[k] = v
end


-- Set the runtime path
--vim.o.runtimepath = vim.o.runtimepath .. ';' .. 'C:/Users/angryteapodthebonedo/AppData/Local/nvim'

-- load packer
require('packer').init()

-- source packer configuration
require('packer')

-- source plugin definitions
require('config.plugins')

require('config.keymaps')

-- Enable syntax highlighting
vim.cmd("syntax on")


