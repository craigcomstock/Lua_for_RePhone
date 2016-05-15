h = function (x)
	if x == nil then
		x = _G
	end
	for k,v in pairs(x) do
		print (k,v)
	end
end

-- ls = function (dir)
-- 	os.list(dir)
-- end

-- colors
foreground = 255
background = 0

clear = function(color)
	screen.clear(color)
	screen.update()
end

VM_GRAPHIC_ROTATE_NORMAL=0
VM_GRAPHIC_ROTATE_90=0x01
VM_GRAPHIC_ROTATE_180=0x02
VM_GRAPHIC_ROTATE_270=0x03
VM_GRAPHIC_MIRROR=0x80
VM_GRAPHIC_MIRROR_90=0x81
VM_GRAPHIC_MIRROR_180=0x82
VM_GRAPHIC_MIRROR_270=0x83

show = function (msg)
	screen.clear(background)
	screen.set_color(foreground)
	screen.text(0,120,msg) -- todo option x,y
	screen.rotate(VM_GRAPHIC_ROTATE_180)
	screen.update()
end

peek = os.peek
poke = os.poke
-- shutdown = os.shutdown
-- reboot = os.reboot

-- vm_malloc = os.vm_malloc
-- vm_free = os.vm_free
-- vm_malloc_dma = os.vm_malloc_dma

-- malloc = os.malloc
-- free = os.free

init = screen.init
reboot_fn,shutdown_fn = os.symbol()
shutdown = function() os.jmp(shutdown_fn) end
reboot = function() os.jmp(reboot_fn) end

-- mem = os.mem

-- todo memtest(), use malloc, vm_malloc and vm_malloc_dma and progressively
-- malloc more and more by += 1024 until the fail... then report max amounts 
-- for each individually...
