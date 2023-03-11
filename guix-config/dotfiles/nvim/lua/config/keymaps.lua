-- Map Ctrl-v to paste from system clipboard in normal and insert mode
vim.api.nvim_set_keymap('n', '<C-v>', '"+p', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-v>', '<Esc>"+pa', { noremap = true })

vim.api.nvim_set_keymap('n', '<M-v>', '<C-v>', { noremap = true })


--vim.api.nvim_set_keymap('n', 'Y', '"+y', { noremap = true })
--vim.api.nvim_set_keymap('v', '<S-Y>', [[:w !clip<CR><CR>]], { noremap = true })

vim.api.nvim_set_keymap('v', 'Y', '"+y', { noremap = true })


--wsl yank to cliboard
