vim.keymap.set('n', '<F7>', ':make<CR>')
vim.keymap.set('n', '<F5>', ':!build\\bin\\FlexLayoutDesigner.exe<CR>')
vim.o.makeprg='cmake --build build'


