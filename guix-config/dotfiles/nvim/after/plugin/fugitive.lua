vim.keymap.set("n", "<leader>gs", vim.cmd.Git);

vim.keymap.set("n", "<leader>gc", function()
  local message = vim.fn.input("Commit message: ")
  local add_output = vim.fn.systemlist("git add .")
  if #add_output > 0 then
    vim.api.nvim_err_writeln("Error running git add: " .. table.concat(add_output, "\n"))
    return
  end
  local commit_output = vim.fn.systemlist("git commit -m '" .. message .. "'")
  if #commit_output > 0 then
    vim.api.nvim_err_writeln("Error running git commit: " .. table.concat(commit_output, "\n"))
    return
  end

  local git_diff_output = vim.fn.systemlist("git diff --shortstat HEAD")
  local git_diff_stats = git_diff_output[1]
  local git_diff_stats_color = ""
  if string.match(git_diff_stats, "%d+") then
    local added_lines, removed_lines, files_changed = string.match(git_diff_stats, "(%d+)%D+(%d+)%D+(%d+)")
    git_diff_stats_color = "%#DiffAdd#+" .. added_lines .. "%#DiffDelete#-" .. removed_lines .. "%#DiffText# ~ " .. files_changed .. " files"
  else
    git_diff_stats_color = "%#DiffText#No changes"
  end
  vim.api.nvim_out_write("Git commit successful! " .. git_diff_stats_color .. "\n")
end)

vim.keymap.set("n", "<leader>gp", function()
  vim.cmd("G push origin master")
  vim.api.nvim_out_write("Git push successful!\n")
end)

