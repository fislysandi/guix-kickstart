vim.keymap.set("n", "<leader>gs", vim.cmd.Git);

vim.keymap.set("n", "<leader>g", function()
  local message = vim.fn.input("Commit message: ")
  vim.api.nvim_command("G add .")
  vim.api.nvim_command("G commit -m '" .. message .. "'")
  vim.api.nvim_out_write("Git commit successful!\n")
end)


vim.keymap.set("n", "<leader>gp", function()
  vim.cmd("G push origin master")
  vim.api.nvim_out_write("Git push successful!\n")
end)

