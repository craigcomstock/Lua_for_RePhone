foreground = 0xffffff
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

-- TODO make show wrap characters and start top-left!?
show = function (msg)
	screen.clear(background)
	screen.set_color(foreground)
	screen.text(0,120,msg) -- todo option x,y
	screen.rotate(VM_GRAPHIC_ROTATE_180)
	screen.update()
end

screen.init()
screen.set_brightness(50)

dim = function()
	screen.set_brightness(5)
end
bright = function()
	screen.set_brightness(60)
end
-- TODO make this execute on a timeout!!!
off = function()
	screen.set_brightness(0)
end

touch = {};
touch_i = 0;

screen.touch( function(t,x,y) 
	print(t,x,y);
	if (t==4) then
		touch_i = 0;
		touch = {};
		if (yes ~= nil) then
			yes();
		end
	end

	if (t==1) then
		touch_i = 0;
		touch = {};
	end

	touch_i = touch_i + 1;

	touch[touch_i] = {t=t, x=x, y=y}
	if (t==2) then
		ongesture(touch);
	end
end)

ongesture = function(touches)

	for i,touch in pairs(touches) do
		print(touch.t,touch.x,touch.y);
	end

	if ((#touches == 2) and 
		(touches[1].t == 1) and
		(touches[2].t == 2)) then
		if (no ~= nil) then no(); end
	end
	
end
