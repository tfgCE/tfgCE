#include "http.h"

#include "netClientTCP.h"

#include "..\containers\list.h"
#include "..\other\parserUtils.h"
#include "..\types\string.h"

using namespace Network;

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

String HTTP::get(tchar const* _address)
{
	String address(_address);
	if (address.has_prefix(TXT("http://")))
	{
		address = address.get_sub(7);
	}
	if (String::does_contain(address.to_char(), TXT(":/")))
	{
		error(TXT("can't handle this kind of address"));
		todo_important(TXT("implement!"));
		return String::empty();
	}
	if (address.has_suffix(TXT("/")))
	{
		address = address.get_left(address.get_length() - 1);
	}
	
	String content;
	{
		int at = NONE;
		{
			int tat = address.find_first_of('\\');
			if (tat != NONE && (at == NONE || tat < at))
			{
				at = tat;
			}
		}
		{
			int tat = address.find_first_of('/');
			if (tat != NONE && (at == NONE || tat < at))
			{
				at = tat;
			}
		}
		if (at == NONE)
		{
			content = TXT("/");
		}
		else
		{
			content = address.get_sub(at);
			address = address.get_left(at);
		}
	}
	::Network::ClientTCP client(::Network::Address(address, 80));
	client.connect();
	if (client.has_connection_failed())
	{
		error(TXT("could not connect to \"%S\""), address.to_char());
		return String::empty();
	}
	{	// send
		String getMessage = String::printf(TXT("GET %S HTTP/1.1\nHost: %S\n\n"), content.to_char(), address.to_char());
		client.send_raw_8bit_text(getMessage);
	}
	{	// process response
		String response;
		if (client.recv_raw_8bit_text(response))
		{
			int responseCode = 0;
			int spaceAt = response.find_first_of(' ');
			if (spaceAt != NONE)
			{
				++spaceAt;
				String responseCodeString;
				while (response[spaceAt] >= '0' && response[spaceAt] <= '9')
				{
					responseCodeString += response[spaceAt];
					++spaceAt;
				}
				responseCode = ParserUtils::parse_int(responseCodeString);
			}
			if (responseCode == 200 /* ok */ ||
				responseCode == 201 /* created */ ||
				responseCode == 202 /* accepted */)
			{
				response = response.replace(String(TXT("\r\n")), String(TXT("\n")));
				List<String> lines;
				response.split(String(TXT("\n")), lines);
				int emptyLineAt = NONE;
				for_every(line, lines)
				{
					if (line->is_empty())
					{
						emptyLineAt = for_everys_index(line);
						break;
					}
				}
				if (emptyLineAt != NONE && emptyLineAt < lines.get_size() - 1)
				{
					String result;
					int lineAt = emptyLineAt + 1;
					while (lineAt < lines.get_size())
					{
						if (lines[lineAt].has_prefix(TXT("HTTP")))
						{
							break;
						}
						result += lines[lineAt];
						result += TXT("\n");
						++lineAt;
					}
					return result;
				}
				else
				{
					error(TXT("no response"));
					return String::empty();
				}
			}
			else
			{
				error(TXT("invalid response (http:%i)"), responseCode);
				return String::empty();
			}
		}
		else
		{
			error(TXT("could not receive response"));
			return String::empty();
		}
	}
}
