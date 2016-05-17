call = gsm.call
hang = gsm.hang
hangup = hang
answer = gsm.accept
text = gsm.text -- num,msg
gsm.on_incoming_call( function(phone_number)
	-- ring.start();
	-- save_call(phone_number)
	yes = function()
		answer()
	end
	no = function()
		hangup()
	end
end)

gsm.on_new_message( function (phone_number, message)
	-- chirp()
	-- save message
	show(phone_number+':'+message)
end)
