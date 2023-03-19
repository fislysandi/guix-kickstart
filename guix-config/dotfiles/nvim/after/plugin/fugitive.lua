vim.keymap.set("n", "<leader>gs", vim.cmd.Git);

vim.keymap.set("n", "<leader>gc", function()
  local message = vim.fn.input("Commit message: ")
  vim.api.nvim_command("G add *")
  vim.api.nvim_command("G commit -m '" .. message .. "'")
end)


vim.keymap.set("n", "<leader>gp", function()
  vim.cmd("G push origin master")
 end)

