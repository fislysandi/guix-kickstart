vim.keymap.set("n", "<leader>gs", vim.cmd.Git);

vim.keymap.set("n", "<leader>gc", function()
  local message = vim.fn.input("Commit message: ")
  vim.cmd("Gwrite")
  vim.cmd("G commit -m '" .. message .. "'")
  vim.api.nvim_out_write("Git commit successful!\n")
end)

vim.keymap.set("n", "<leader>gp", function()
  vim.cmd("Gpush")
  vim.api.nvim_out_write("Git push successful!\n")
end)

