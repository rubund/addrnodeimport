
out = {}

local address = true
local letter = ""

for i, indent, tokens in tokens, info, 0 do
	if tokens[1] == "OBJTYPE" then
		if tokens[2] == "Vegadresse" then
			address = true
		elseif tokens[2] == "Matrikkeladresse" then
			address = false
		end
	elseif tokens[1] == "ADRESSENAVN" then
		out["addr:street"] = tokens[2]
	elseif tokens[1] == "NUMMER" then
		out["addr:housenumber"] = tokens[2]
	elseif tokens[1] == "BOKSTAV" then
		letter = tokens[2]
	elseif tokens[1] == "POSTNUMMER" then
		out["addr:postcode"] = tokens[2]
	elseif tokens[1] == "POSTSTED" then
		out["addr:city"] = initcase(tokens[2])
	end
end

if out["addr:housenumber"] and letter then out["addr:housenumber"] = string.format("%s%s", out["addr:housenumber"], letter) end

if address == false then
	return {}
end

return out
