-- Map Ctrl-v to paste from system clipboard in normal and insert mode
vim.api.nvim_set_keymap('n', '<C-v>', '"+p', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-v>', '<Esc>"+pa', { noremap = true })

vim.api.nvim_set_keymap('n', '<M-v>', '<C-v>', { noremap = true })


--vim.api.nvim_set_keymap('n', 'Y', '"+y', { noremap = true })
--vim.api.nvim_set_keymap('v', '<S-Y>', [[:w !clip<CR><CR>]], { noremap = true })

vim.api.nvim_set_keymap('v', 'Y', '"+y', { noremap = true })

vim.g.mapleader = " "

vim.keymap.set('n', '<leader>fe', vim.cmd.Ex)

vim.keymap.set("v", "J", ":m '>+1<CR>gv=gv")
vim.keymap.set("v", "K", ":m '<-2<CR>gv=gv")
--wsl yank to cliboard

vim.keymap.set("n", "<C-k>", "<cmd>cnext<CR>zz")
vim.keymap.set("n", "<C-j>", "<cmd>cprev<CR>zz")
vim.keymap.set("n", "<leader>k", "<cmd>lnext<CR>zz")
vim.keymap.set("n", "<leader>j", "<cmd>lprev<CR>zz")

-- replace all instances of a word
vim.keymap.set("n", "<leader>s", [[:%s/\<<C-r><C-w>\>/<C-r><C-w>/gI<Left><Left><Left>]])


vim.keymap.set("n", "<leader>x", "<cmd>!chmod +x %<CR>", { silent = true })


vim.keymap.set("n", "<leader>gc", function()
  local message = vim.fn.input("Commit message: ")
  local add_output = vim.fn.systemlist("git add . 2>&1")
  local add_success = vim.v.shell_error == 0
  local commit_output = vim.fn.systemlist("git commit -m '" .. message .. "' 2>&1")
  local commit_success = vim.v.shell_error == 0

  if not add_success then
    vim.api.nvim_err_writeln("Error running git add: " .. table.concat(add_output, "\n"))
    return
  end

  if not commit_success then
    vim.api.nvim_err_writeln("Error running git commit: " .. table.concat(commit_output, "\n"))
    return
  end

  vim.api.nvim_out_write("Git commit successful!\n")
end)

vim.keymap.set("n", "<leader>gp", function()
  local push_output = vim.fn.systemlist("git push origin HEAD:master")
  if #push_output > 0 then
    vim.api.nvim_err_writeln("Error running git push: " .. table.concat(push_output, "\n"))
    return
  end
  vim.api.nvim_out_write("Git push successful!\n")
end)


