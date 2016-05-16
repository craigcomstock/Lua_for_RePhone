call = gsm.call
hang = hangup = gsm.hang
answer = gsm.accept
gsm.on_incoming_call( function(phone_number)
	-- ring.start();
	-- save_call(phone_number)
	yes = function()
		answer()
	end
	no = function()
		hangup()
	end
end
gsm.on_new_message( function (phone_number, message)
	-- chirp()
	-- save message
	show(phone_number,message)
end
