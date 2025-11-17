#include "rest.h"

#include "..\..\core\utils.h"
#include "..\..\core\network\network.h"
#include "..\..\core\network\netClientTCP.h"
#include "..\..\core\other\parserUtils.h"

using namespace Framework;

tchar const* authorizationToken = TXT("XwEgaPEFwmSkBVol_MIL6YuaJQdHeib5");

RefCountObjectPtr<IO::JSON::Document> REST::send(tchar const* _address, tchar const* _command, IO::JSON::Document const * _message, std::function<void(float _progress)> _update_progress)
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
		return RefCountObjectPtr<IO::JSON::Document>();
	}
	if (address.has_suffix(TXT("/")))
	{
		address = address.get_left(address.get_length() - 1);
	}

	String command(_command);
	if (!command.has_prefix(TXT("/")))
	{
		error(TXT("command should start with /"));
		return RefCountObjectPtr<IO::JSON::Document>();
	}
	if (!command.has_suffix(TXT("/")))
	{
		command += TXT("/");
	}

	int slashAt = address.find_first_of('/');
	if (slashAt != NONE)
	{
		command = address.get_sub(slashAt) + command;
		address = address.get_left(slashAt);
		command = command.replace(String(TXT("//")), String(TXT("/")));
	}

	Network::ClientTCP client(Network::Address(address, 80));
	client.connect();
	if (client.has_connection_failed())
	{
		error(TXT("could not connect to \"%S\""), address.to_char());
		return RefCountObjectPtr<IO::JSON::Document>();
	}
	bool processResponse = false;
	{	// send
		String jsonText = _message->to_string();

		String postMessage = String::printf(TXT("POST %S HTTP/1.1\nHost: %S\nAuthorization: %S\nContent-Type: application/json\nContent-Length: %i\n\n"),
			command.to_char(), address.to_char(), authorizationToken, jsonText.get_length());
		postMessage += jsonText; // we won't fit into the formatting buffer
		if (_update_progress)
		{
			int at = 0;
			int blockSize = 4096;
			int wholeSize = postMessage.get_length();
			while (at < wholeSize)
			{
				if (_update_progress)
				{
					_update_progress(clamp((float)at / (float)wholeSize, 0.0f, 1.0f));
				}
				int nextAt = at + blockSize;
				processResponse = client.send_raw_8bit_text(postMessage.get_sub(at, blockSize), nextAt >= wholeSize /* send null terminator */);
				at = nextAt;
				if (_update_progress)
				{
					_update_progress(clamp((float)at / (float)wholeSize, 0.0f, 1.0f));
				}
			}
		}
		else
		{
			processResponse = client.send_raw_8bit_text(postMessage);
		}
	}
	if (processResponse)
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
					RefCountObjectPtr<IO::JSON::Document> document(new IO::JSON::Document());
					if (lines[emptyLineAt + 1].has_prefix(TXT("HTTP")))
					{
						return document;
					}
					else
					{
						if (document->parse(lines[emptyLineAt + 1]))
						{
							return document;
						}
						else
						{
							error(TXT("could not parse"));
							return RefCountObjectPtr<IO::JSON::Document>();
						}
					}
				}
				else
				{
					error(TXT("no response"));
					return RefCountObjectPtr<IO::JSON::Document>();
				}
			}
			else
			{
				error(TXT("invalid response (rest:%i)"), responseCode);
				return RefCountObjectPtr<IO::JSON::Document>();
			}
		}
		else
		{
			error(TXT("could not receive response"));
			return RefCountObjectPtr<IO::JSON::Document>();
		}
	}
	else
	{
		error(TXT("problem posting to rest"));
		return RefCountObjectPtr<IO::JSON::Document>();
	}
}
