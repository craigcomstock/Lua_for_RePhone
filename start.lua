h = function (x)
	if x == nil then
		x = _G
	end
	for k,v in pairs(x) do
		print (k,v)
	end
end

-- yes and no functions can be redefined for various questions
yes = function() -- noop
end
no = function() -- noop
end

dofile('call.lua')
dofile('screen.lua')
dofile('os.lua')
