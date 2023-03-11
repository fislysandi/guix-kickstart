-- packer.lua

-- configure packer
return require('packer').init({
  -- add your packer configuration options here
  git = {
    clone_timeout = 600,
  },
})

