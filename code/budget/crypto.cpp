#include "crypto.h"

#include "..\core\math\math.h"
#include "..\core\io\dir.h"
#include "..\core\io\file.h"

//

DEFINE_STATIC_NAME(PLN);
DEFINE_STATIC_NAME(USD);

DEFINE_STATIC_NAME(BTC);
DEFINE_STATIC_NAME(ETH);
DEFINE_STATIC_NAME(BNB);

DEFINE_STATIC_NAME(binance);
DEFINE_STATIC_NAME(bitbay);
DEFINE_STATIC_NAME(coinroom);

bool immediateFee = true;

//#define TEST_TRANS

/*
 *
 *	TODO
 *	auto transfer of small values was applied a while ago
 *	I need to change that by hand for 2018?
 *
 */
void process_crypto(String const & _dirInput, String const & _fileOut)
{
	Budget::Crypto crypto;

	crypto.load_dir(_dirInput);

	crypto.process(_fileOut);
}

using namespace Budget;

#ifdef TEST_TRANS
int testId = 0;
int opId = 0;
#endif

void CryptoQueue::add(IO::File & fOut, Name const & _market, Name const & _crypto, CryptoInQueue const & _provided)
{
	queue.push_back(_provided);
#ifdef TEST_TRANS
	if (_market == NAME(binance) && _crypto == NAME(ETH))
	{
		fOut.write_line(String::printf(TXT("!@# %i %S add %14s %S : has %14s %S"), testId, _market.to_char(), _provided.amount.to_string(8).to_char(), _crypto.to_char(), get_amount().to_string().to_char(), _crypto.to_char()));
		++testId;
	}
#endif
}

void CryptoQueue::use(IO::File & fOut, Name const & _market, Name const & _crypto, REF_ Number & _amount, OUT_ CryptoInQueue & _provided)
{
	_provided = CryptoInQueue();
	if (queue.is_empty())
	{
		return;
	}
	auto& topQueue = queue.get_first();

	if (_amount >= topQueue.amount)
	{
#ifdef TEST_TRANS
		output(TXT("_amount : %S"), _amount.to_string().to_char());
		output(TXT("topQueue.amount : %S"), topQueue.amount.to_string().to_char());
#endif
		_provided = topQueue;
		_amount -= topQueue.amount;
		queue.pop_front();
#ifdef TEST_TRANS
		output(TXT("_amount left : %S"), _amount.to_string().to_char());
#endif
	}
	else
	{
#ifdef TEST_TRANS
		if (opId == 1109)
		{
			fOut.flush();
			AN_BREAK;
		}
#endif
		// use only percentage
		Number usePt = _amount / topQueue.amount;
#ifdef TEST_TRANS
		output(TXT("usePt : %S"), usePt.to_string().to_char());
		output(TXT("topQueue.amount : %S"), topQueue.amount.to_string().to_char());
		output(TXT("_amount : %S"), _amount.to_string().to_char());
#endif
		// always reduce amount but cost & value should be reduced using percentage
		topQueue.amount -= _amount;
#ifdef TEST_TRANS
		output(TXT("topQueue.amount left : %S"), topQueue.amount.to_string().to_char());
#endif
		_provided.amount = _amount;
		_provided.costPLN = topQueue.costPLN * usePt;
		_provided.valuePLN = topQueue.valuePLN * usePt;
		_provided.costPLN.limit_point_at_to(2);
		_provided.valuePLN.limit_point_at_to(2);
		topQueue.costPLN -= _provided.costPLN;
		topQueue.valuePLN -= _provided.valuePLN;
		_amount = Number(); // zero
#ifdef TEST_TRANS
		if (_market == NAME(binance) && _crypto == NAME(ETH))
		{
			Number test;
			test.parse(String(TXT("0.00874")));
			if (topQueue.amount == test)
			{
				fOut.flush();
				AN_BREAK;
			}
		}
		++opId;
#endif
	}
#ifdef TEST_TRANS
	if (_market == NAME(binance) && _crypto == NAME(ETH))
	{
		if (testId == 20)
		{
			AN_BREAK;
		}
		fOut.write_line(String::printf(TXT("!@# %i %S rem %14s %S : has %14s %S"), testId, _market.to_char(), _provided.amount.to_string(8).to_char(), _crypto.to_char(), get_amount().to_string().to_char(), _crypto.to_char()));
		++testId;
	}
#endif
}

Number CryptoQueue::get_amount() const
{
	Number amount;
	for_every(ciq, queue)
	{
		amount += ciq->amount;
	}
	return amount;
}

//

bool Crypto::load_dir(String const & _dir)
{
	bool result = true;
	IO::Dir dir;
	if (dir.list(_dir))
	{
		Array<String> const & directories = dir.get_directories();
		for_every(directory, directories)
		{
			result &= load_dir(_dir + TXT("/") + *directory);
		}

		Array<String> const & files = dir.get_files();
		for_every(file, files)
		{
			output(TXT("  %S"), file->to_char());
			if (file->has_suffix(String(TXT(".csv"))))
			{
				if (file->has_prefix(TXT("USD")))
				{
					String currency = file->get_sub(3, 3);
					output(TXT("    +-> loading USD exchange rate %S"), currency.to_char());
					result &= load_usd_rates_csv(&IO::File(_dir + TXT("\\") + *file), currency);
				}
				else if (file->has_prefix(TXT("PLNUSD")))
				{
					output(TXT("    +-> loading PLN exchange rate USD"));
					result &= load_pln_usd_rates_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("binance_2")))
				{
					output(TXT("    +-> loading binance_2"));
					result &= load_binance_2_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("binance")))
				{
					output(TXT("    +-> loading binance"));
					result &= load_binance_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("bitbay")))
				{
					output(TXT("    +-> loading bitbay"));
					result &= load_bitbay_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("coinroom")))
				{
					output(TXT("    +-> loading coinroom"));
					result &= load_coinroom_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("transfer")))
				{
					output(TXT("    +-> loading transfer"));
					result &= load_transfer_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else if (file->has_prefix(TXT("extra gain")))
				{
					output(TXT("    +-> loading extra gain"));
					result &= load_extra_gain_csv(&IO::File(_dir + TXT("\\") + *file));
				}
				else
				{
					error(TXT("can't load file \"%S\""), file->to_char());
					result = false;
				}
			}
			else
			{
				error(TXT("can't load file \"%S\""), file->to_char());
				result = false;
			}
		}
	}

	return result;
}

String load_till(IO::File* _file, tchar const * _tillChars, bool _readChar = true)
{
	String result;
	if (_readChar)
	{
		_file->read_char();
	}
	bool quotes = _file->get_last_read_char() == '"';
	while (_file->has_last_read_char())
	{
		bool found = false;
		for_count(int, i, (int)tstrlen(_tillChars))
		{
			if (_file->get_last_read_char() == _tillChars[i])
			{
				found = true;
				break;
			}
		}
		if (found ||
			_file->get_last_read_char() == '\n' ||
			_file->get_last_read_char() == '\r')
		{
			break;
		}
		result += _file->get_last_read_char();
		_file->read_char();
	}
	if (quotes)
	{
		result = result.get_sub(1, result.get_length() - 2);
	}
	return result;
}

bool Crypto::load_pln_usd_rates_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	String headerStr;
	for_count(int, i, 16)
	{
		headerStr += _file->read_char();
	}
	_file->read_next_line();
	if (headerStr == TXT("Data;Kurs;Kod IS"))
	{
		// https://www.investing.com/currencies/usd-pln-historical-data
		// tab -> ;
		// Data;Kurs;Kod ISO z;Kod ISO na
		an_assert(rawOperations.is_empty());
		while (true)
		{
			String dateStr = load_till(_file, TXT(";"), false);
			String priceStr = load_till(_file, TXT(";"));

			if (!_file->has_last_read_char())
			{
				break;
			}

			Date date;
			date.parse_dd_mm_yyyy_dot(dateStr.get_left(10));

			Number price;
			price.parse(priceStr);

			CryptoExchangeRate er;
			er.date = date;
			er.crypto = NAME(USD);
			er.pricePLN = price;
			exchangeRatesPLN.push_back(er);

			_file->read_next_line();
		}
	}
	else
	{
		// https://www.investing.com/currencies/usd-pln-historical-data
		// tab -> ;
		// "Date","Price","Open","High","Low","Change %"
		an_assert(rawOperations.is_empty());
		while (true)
		{
			Date date;
			date.load_from_month_dd_yyyy(_file);
			_file->read_char();
			if (!_file->has_last_read_char())
			{
				break;
			}
			if (_file->get_last_read_char() != ';' &&
				_file->get_last_read_char() != ',')
			{
				error(TXT("syntax error (pln/usd)"));
				return false;
			}

			String priceStr = load_till(_file, TXT(";,"));

			Number price;
			price.parse(priceStr.replace(String(TXT("\"")), String::empty()));

			CryptoExchangeRate er;
			er.date = date;
			er.crypto = NAME(USD);
			er.pricePLN = price;
			exchangeRatesPLN.push_back(er);

			_file->read_next_line();
		}
	}

	return result;
}

bool Crypto::load_usd_rates_csv(IO::File* _file, String const & _currency)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// https://coinmarketcap.com/currencies/bitcoin/historical-data/
	// copy to google sheets, export as .tsv
	// replace "tab" with ";"
	// Date; Open; High; Low; Close; Volume; Market Cap
	_file->read_next_line();

	Name currency(_currency);

	an_assert(rawOperations.is_empty());
	while (true)
	{
		Date date;
		date.load_from_month_dd_yyyy(_file);
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';')
		{
			error(TXT("syntax error (usd rates)"));
			return false;
		}

		String openStr = load_till(_file, TXT(";"));
		String highStr = load_till(_file, TXT(";"));
		String lowStr = load_till(_file, TXT(";"));
		String closeStr = load_till(_file, TXT(";"));
		String volumeStr = load_till(_file, TXT(";"));
		String marketCapStr = load_till(_file, TXT(";"));

		Number open;
		open.parse(openStr);

		CryptoExchangeRateUSD er;
		er.date = date;
		er.crypto = currency;
		er.priceUSD = open;
		exchangeRatesUSD.push_back(er);

		_file->read_next_line();
	}

	return result;
}

bool Crypto::load_coinroom_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// "Waluta cyfrowa";"Real";"Kwota";"Kwota do wykorzystania";"Kurs";"Cena";"Prowizje";"Typ";"Status";"Wystawiona";"";
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		String currencyStr = load_till(_file, TXT(";"), false);
		String fiatStr = load_till(_file, TXT(";"));
		String amountStr = load_till(_file, TXT(";"));
		load_till(_file, TXT(";"));
		String priceStr = load_till(_file, TXT(";"));
		String valueStr = load_till(_file, TXT(";"));
		String feeStr = load_till(_file, TXT(";"));
		String typeStr = load_till(_file, TXT(";"));
		String stateStr = load_till(_file, TXT(";"));

		// enter date field
		_file->read_char();
		Date date;
		date.load_from_dd_mm_yyyy_hh_mm_ss(_file);
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';')
		{
			error(TXT("syntax error (coinroom)"));
			return false;
		}
		load_till(_file, TXT(";"));

		Number amount;
		amount.parse(amountStr);
		amount.make_absolute();
		Number value;
		value.parse(valueStr);
		value.make_absolute();
		Number fee;
		fee.parse(feeStr);
		fee.make_absolute();

		Name currency(currencyStr);
		if (fiatStr != TXT("PLN"))
		{
			error(TXT("PLN only"));
			result = false;
		}

		if (stateStr == TXT("Zamkniêta w ca³oœci"))
		{
			if (typeStr == TXT("Bid"))
			{
				CryptoOperation op;
				op.date = date;
				op.id = ++id;
				op.type = CryptoOperation::Buy;
				op.market = NAME(coinroom);
				CryptoCurrencyOperationValue a, b, cost;
				a.name = currency;
				a.value = amount;
				a.valuePLN = value;
				b.name = NAME(PLN);
				b.value = value;
				b.valuePLN = value;
				cost.name = currency;
				cost.value = fee * amount / value;
				cost.valuePLN = fee;
				op.a = a;
				op.b = b;
				op.cost = cost;
				operations.push_back(op);
			}
			else if (typeStr == TXT("Ask"))
			{
				CryptoOperation op;
				op.date = date;
				op.id = ++id;
				op.type = CryptoOperation::Sell;
				op.market = NAME(coinroom);
				CryptoCurrencyOperationValue a, b, cost;
				a.name = currency;
				a.value = amount;
				a.valuePLN = value;
				b.name = NAME(PLN);
				b.value = value;
				b.valuePLN = value;
				cost.name = NAME(PLN);
				cost.value = fee;
				cost.valuePLN = fee;
				op.a = a;
				op.b = b;
				op.cost = cost;
				operations.push_back(op);
			}
		}
		else if (stateStr == TXT("Anulowana"))
		{
			// ok
		}
		else
		{
			error(TXT("not handled"));
			result = false;
		}

		_file->read_next_line();
	}

	return result;
}

bool Crypto::load_transfer_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// Data transferu;Waluta;Sk¹d;Dok¹d;Zesz³o;Dosz³o;
	// this has to be manually edited using transfer info from bitbay and transfer e-mails from binance
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		Date date;
		date.load_from_dd_mm_yyyy_hh_mm_ss(_file);
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';')
		{
			error(TXT("syntax error (transfer)"));
			return false;
		}

		String currencyStr = load_till(_file, TXT(";"));
		String fromStr = load_till(_file, TXT(";"));
		String toStr = load_till(_file, TXT(";"));
		String amountOutStr = load_till(_file, TXT(";"));
		String amountInStr = load_till(_file, TXT(";"));

		Number amountOut;
		amountOut.parse(amountOutStr);
		amountOut.make_absolute();
		Number amountIn;
		amountIn.parse(amountInStr);
		amountIn.make_absolute();

		Number amount = amountIn;
		Number fee = amountOut - amountIn;

		Name currency(currencyStr);

		{	// transfer
			CryptoOperation op;
			op.date = date;
			op.id = ++id;
			op.market = Name(fromStr);
			op.destMarket = Name(toStr);
			op.type = CryptoOperation::Transfer;
			op.a.name = currency;
			op.a.value = amount;
			operations.push_back(op);
		}
		{	// spending
			CryptoOperation op;
			op.date = date;
			op.id = ++id;
			op.market = Name(fromStr);
			op.type = CryptoOperation::Spending;
			op.a.name = currency;
			op.a.value = fee;
			operations.push_back(op);
		}

		_file->read_next_line();
	}

	return result;
}

bool Crypto::load_extra_gain_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// Data transferu;Waluta;Dok¹d;Dosz³o;
	// this has to be manually edited to fill missing values, earnings
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		Date date;
		date.load_from_dd_mm_yyyy_hh_mm_ss(_file);
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';')
		{
			error(TXT("syntax error (extra gain)"));
			return false;
		}

		String currencyStr = load_till(_file, TXT(";"));
		String toStr = load_till(_file, TXT(";"));
		String amountInStr = load_till(_file, TXT(";"));

		Number amountIn;
		amountIn.parse(amountInStr);
		amountIn.make_absolute();

		Number amount = amountIn;

		Name currency(currencyStr);

		{	// transfer
			CryptoOperation op;
			op.date = date;
			op.id = ++id;
			op.market = Name(toStr);
			op.type = CryptoOperation::ExtraGain;
			op.a.name = currency;
			op.a.value = amount;
			operations.push_back(op);
		}

		_file->read_next_line();
	}

	return result;
}

bool Crypto::load_binance_csv(IO::File* _file)
{
	bool result = true;
	bool advanceZulu = true;

	_file->set_type(IO::InterfaceType::Text);

	// Date(UTC);Market;Type;Price;Amount;Total;Fee;Fee Coin
	// Date(UTC),Market,Type,Price,Amount,Total,Fee,Fee Coin
	String headerStr;
	for_count(int, i, 16)
	{
		headerStr += _file->read_char();
	}
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		Date date;
		date.load_from_yyyy_mm_dd_hh_mm_ss(_file);
		if (advanceZulu)
		{
			date.advance_hour(); // zulu
		}
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';' &&
			_file->get_last_read_char() != ',')
		{
			error(TXT("syntax error (binance)"));
			return false;
		}

		String cryptoPairStr = load_till(_file, TXT(";,"));
		String operationStr = load_till(_file, TXT(";,"));
		String priceStr = load_till(_file, TXT(";,"));
		String amountStr = load_till(_file, TXT(";,"));
		String valueStr = load_till(_file, TXT(";,"));
		String feeStr = load_till(_file, TXT(";,"));
		String feeCurrencyStr = load_till(_file, TXT(";,"));

		Name currencyMain(cryptoPairStr.get_right(3));
		Name currency(cryptoPairStr.get_left(cryptoPairStr.get_length() - 3));
		if (currencyMain != NAME(BTC) &&
			currencyMain != NAME(ETH) &&
			currencyMain != NAME(BNB))
		{
			error(TXT("add support for currency"));
			result = false;
		}

		Number amount; // currency
		amount.parse(amountStr);
		amount.make_absolute();
		Number value; // main currency
		value.parse(valueStr);
		value.make_absolute();
		Number fee; // anything
		fee.parse(feeStr);
		fee.make_absolute();
		Name feeCurrency(feeCurrencyStr);

		if (operationStr == TXT("BUY"))
		{
			CryptoOperation op;
			op.date = date;
			op.id = ++id;
			op.type = CryptoOperation::Buy;
			op.market = NAME(binance);
			CryptoCurrencyOperationValue a, b, cost;
			a.name = currency;
			a.value = amount;
			b.name = currencyMain;
			b.value = value;
			cost.name = feeCurrency;
			cost.value = fee;
			op.a = a;
			op.b = b;
			op.cost = cost;
			operations.push_back(op);
		}
		else if (operationStr == TXT("SELL"))
		{
			CryptoOperation op;
			op.date = date;
			op.id = ++id;
			op.type = CryptoOperation::Sell;
			op.market = NAME(binance);
			CryptoCurrencyOperationValue a, b, cost;
			a.name = currency;
			a.value = amount;
			b.name = currencyMain;
			b.value = value;
			cost.name = feeCurrency;
			cost.value = fee;
			op.a = a;
			op.b = b;
			op.cost = cost;
			operations.push_back(op);
		}
		else
		{
			error(TXT("not handled"));
			result = false;
		}

		_file->read_next_line();
	}

	return result;
}

bool Crypto::load_binance_2_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// User_ID;UTC_Time;Account;Operation;Coin;Change;Remark
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		String userID = load_till(_file, TXT(";"));
		String dateTimeStr = load_till(_file, TXT(";"));
		String accountStr = load_till(_file, TXT(";"));
		String operationStr = load_till(_file, TXT(";"));
		String currencyStr = load_till(_file, TXT(";"));
		String valueStr = load_till(_file, TXT(";"));
		String remarkStr = load_till(_file, TXT(";"));

		Number value;
		value.parse(valueStr);
		Name currency(currencyStr);

		CryptoOperationRaw op;
		op.date.parse_yyyy_mm_dd_minus_hh_mm_ss(dateTimeStr);
		op.id = ++id;
		op.market = NAME(binance);
		if (operationStr == TXT("----?----"))
		{
			op.type = CryptoOperationRaw::InfoTransferIn;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Withdraw"))
		{
			op.type = CryptoOperationRaw::InfoTransferOut;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Buy") &&
			value.is_positive())
		{
			op.type = CryptoOperationRaw::BuyIn;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Buy") &&
			! value.is_positive())
		{
			op.type = CryptoOperationRaw::BuyOut;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Transaction Related") &&
			! value.is_positive())
		{
			op.type = CryptoOperationRaw::BuyOut;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Sell") &&
			value.is_positive())
		{
			op.type = CryptoOperationRaw::SellIn;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Sell") &&
			!value.is_positive())
		{
			op.type = CryptoOperationRaw::SellOut;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Small assets exchange BNB") &&
			value.is_positive())
		{
			op.type = CryptoOperationRaw::SellIn;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Small assets exchange BNB") &&
			!value.is_positive())
		{
			op.type = CryptoOperationRaw::SellOut;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Super BNB Mining"))
		{
			op.type = CryptoOperationRaw::ExtraGain;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Fee"))
		{
			if (value.is_positive())
			{
				error(TXT("fee should not be positive, should always deduct"));
				result = false;
			}
			op.type = CryptoOperationRaw::Fee;
			op.value.value = value.absolute();
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (accountStr == TXT("Savings"))
		{
			// ignore savings
		}
		else if (operationStr.is_empty())
		{
			// empty line?
		}
		else
		{
			error(TXT("operation not recognised \"%S\""), operationStr.to_char());
			result = false;
		}

		_file->read_next_line();
		if (!_file->has_last_read_char())
		{
			break;
		}
	}

	result &= process_raw_operations_binance_2();

	return result;
}

bool Crypto::process_raw_operations_binance_2()
{
	bool result = true;

	Array<CryptoOperation> newOperations;

	sort(rawOperations);

	// first just get transfers
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::InfoTransferIn)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::InfoTransferIn;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
		if (rawOp->type == CryptoOperationRaw::InfoTransferOut)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::InfoTransferOut;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}

	// buying crypto
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::BuyIn)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::Buy;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}
	sort(newOperations);
	// get what we bought for
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::BuyOut)
		{
			int closestIdx = NONE;
			for_every(op, newOperations)
			{
				if (op->type == CryptoOperation::Buy &&
					!op->b.is_set() &&
					abs(Date::difference_between(rawOp->date, op->date)) == 0)
				{
					closestIdx = for_everys_index(op);
					break;
				}
			}
			if (closestIdx != NONE)
			{
				newOperations[closestIdx].b = rawOp->value;
				rawOp->processed = true;
			}
			if (!rawOp->processed)
			{
				error(TXT("what did we buy with?"));
				result = false;
			}
		}
	}

	// selling crypto
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::SellOut)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::Sell;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}
	// get what we got for selling
	sort(newOperations);
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::SellIn)
		{
			int closestIdx = NONE;
			for_every(op, newOperations)
			{
				if (op->type == CryptoOperation::Sell &&
					!op->b.is_set() &&
					abs(Date::difference_between(rawOp->date, op->date)) == 0)
				{
					closestIdx = for_everys_index(op);
					break;
				}
			}
			if (closestIdx != NONE)
			{
				newOperations[closestIdx].b = rawOp->value;
				rawOp->processed = true;
			}
			if (!rawOp->processed)
			{
				error(TXT("what did we sell for?"));
				result = false;
			}
		}
	}

	// mining etc
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::ExtraGain)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::ExtraGain;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}

	// fee, close
	int maxDists[] = { 0, 1, 5, 10, 30, 80, 100, NONE };
	int maxDistIdx = 0;
	sort(newOperations);
	while (maxDists[maxDistIdx] != NONE)
	{
		int maxDist = maxDists[maxDistIdx];
		bool allOk = false;
		for_every(rawOp, rawOperations)
		{
			if (rawOp->processed)
			{
				continue;

			}
			if (rawOp->type == CryptoOperationRaw::Fee)
			{
				int closestDist = 0;
				int closestIdx = NONE;
				for_every(op, newOperations)
				{
					if (Date::distance_between(op->date, rawOp->date) <= maxDist &&
						!op->cost.is_set() &&
						(op->type == CryptoOperation::Buy || op->type == CryptoOperation::Sell))
					{
						int dist = Date::distance_between(rawOp->date, op->date);
						dist *= 100;
						dist += abs(rawOp->id - op->id);
						if (closestIdx == NONE || dist <= closestDist)
						{
							closestIdx = for_everys_index(op);
							closestDist = dist;
						}
					}
				}
				if (closestIdx != NONE)
				{
					newOperations[closestIdx].cost = rawOp->value;
					rawOp->processed = true;
				}
				if (!rawOp->processed)
				{
					allOk = false;
					if (maxDists[maxDistIdx + 1] == NONE)
					{
						error(TXT("what did we pay for?"));
						result = false;
					}
				}
			}
		}
		++maxDistIdx;
		if (allOk)
		{
			break;
		}
	}

	// sum operations of same type (copied from bitbay which was giving them in random order)
	// here we check for both currencies to match
	for (int i = 0; i < newOperations.get_size(); ++i)
	{
		auto& iop = newOperations[i];
		for (int j = i + 1; j < newOperations.get_size(); ++j)
		{
			auto& jop = newOperations[j];
			int dist = abs(Date::distance_between(iop.date, jop.date));
			if (iop.type == jop.type &&
				dist == 0 &&
				(iop.type == CryptoOperation::Buy ||
				 iop.type == CryptoOperation::Sell) &&
				iop.a.name == jop.a.name &&
				iop.b.is_set() && jop.b.is_set() &&
				iop.b.get().name == jop.b.get().name)
			{
				iop.a.value += jop.a.value;
				iop.a.valuePLN.clear();
				iop.b.access().value += jop.b.get().value;
				iop.b.access().valuePLN.clear();
				iop.cost.access().value += jop.cost.get().value;
				newOperations.remove_at(j);
				--j;
			}
		}
	}

	// move
	operations.add_from(newOperations);

	rawOperations.clear();

	return result;
}

bool Crypto::load_bitbay_csv(IO::File* _file)
{
	bool result = true;

	_file->set_type(IO::InterfaceType::Text);

	// Data operacji;Rodzaj;Wartoœæ;Waluta;Saldo ca³kowite po operacji;
	_file->read_next_line();

	an_assert(rawOperations.is_empty());
	while (true)
	{
		Date date;
		date.load_from_auto_hh_mm_ss(_file);
		_file->read_char();
		if (!_file->has_last_read_char())
		{
			break;
		}
		if (_file->get_last_read_char() != ';')
		{
			error(TXT("syntax error (bitbay)"));
			return false;
		}

		String operationStr = load_till(_file, TXT(";"));
		String valueStr = load_till(_file, TXT(";"));
		String currencyStr = load_till(_file, TXT(";"));
		String notUsedStr = load_till(_file, TXT(";"));

		Number value;
		value.parse(valueStr);
		value.make_absolute();
		Name currency(currencyStr);

		CryptoOperationRaw op;
		op.date = date;
		op.id = ++id;
		op.market = NAME(bitbay);
		if (operationStr == TXT("Wp³ata na rachunek"))
		{
			op.type = CryptoOperationRaw::InfoTransferIn;
			op.value.value = value;
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Wyp³ata œrodków"))
		{
			op.type = CryptoOperationRaw::InfoTransferOut;
			op.value.value = value;
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Otrzymanie œrodków z transakcji na rachunek"))
		{
			op.type = CryptoOperationRaw::In;
			op.value.value = value;
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Pobranie œrodków z transakcji z rachunku"))
		{
			op.type = CryptoOperationRaw::Out;
			op.value.value = value;
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Pobranie prowizji za transakcjê"))
		{
			op.type = CryptoOperationRaw::Fee;
			op.value.value = value;
			op.value.name = currency;
			rawOperations.push_back(op);
		}
		else if (operationStr == TXT("Anulowanie oferty poni¿ej wartoœci minimalnych") ||
			operationStr == TXT("Odblokowanie œrodków") ||
			operationStr == TXT("Blokada œrodków"))
		{
			// ignore
		}
		else if (operationStr == TXT("Utworzenie rachunku"))
		{
			// skip
		}
		else
		{
			error(TXT("operation not recognised \"%S\""), operationStr.to_char());
			result = false;
		}

		_file->read_next_line();
	}

	result &= process_raw_operations_bitbay();

	return result;
}

bool Crypto::process_raw_operations_bitbay()
{
	bool result = true;

	Array<CryptoOperation> newOperations;

	sort(rawOperations);

	// first just get transfers
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::InfoTransferIn)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::InfoTransferIn;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
		if (rawOp->type == CryptoOperationRaw::InfoTransferOut)
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::InfoTransferOut;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}

	// buying crypto with PLN
	// find crypto first
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::In && rawOp->value.name != NAME(PLN))
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::Buy;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}
	sort(newOperations);
	// get PLN now
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::Out && rawOp->value.name == NAME(PLN))
		{
			int closestDist = 0;
			int closestIdx = NONE;
			for_every(op, newOperations)
			{
				if (op->type == CryptoOperation::Buy &&
					!op->b.is_set() &&
					Date::difference_between(rawOp->date, op->date) <= 5) // we had to pay in front
				{
					int dist = Date::distance_between(rawOp->date, op->date);
					dist *= 100;
					dist += abs(rawOp->id - op->id);
					if (closestIdx == NONE || dist <= closestDist)
					{
						closestIdx = for_everys_index(op);
						closestDist = dist;
					}
				}
			}
			if (closestIdx != NONE)
			{
				newOperations[closestIdx].b = rawOp->value;
				rawOp->processed = true;
			}
			if (!rawOp->processed)
			{
				error(TXT("what did we buy? %S"), rawOp->date.to_string_universal().to_char());
				result = false;
			}
		}
	}

	// selling crypto with PLN
	// find crypto first
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::Out && rawOp->value.name != NAME(PLN))
		{
			CryptoOperation op;
			op.date = rawOp->date;
			op.id = rawOp->id;
			op.market = rawOp->market;
			op.type = CryptoOperation::Sell;
			op.a = rawOp->value;
			newOperations.push_back(op);
			rawOp->processed = true;
		}
	}
	// get PLN now
	sort(newOperations);
	for_every(rawOp, rawOperations)
	{
		if (rawOp->processed)
		{
			continue;

		}
		if (rawOp->type == CryptoOperationRaw::In && rawOp->value.name == NAME(PLN))
		{
			int closestDist = 0;
			int closestIdx = NONE;
			for_every(op, newOperations)
			{
				if (op->type == CryptoOperation::Sell &&
					!op->b.is_set() &&
					Date::difference_between(rawOp->date, op->date) >= -5) // we received money afterwards
				{
					int dist = Date::distance_between(rawOp->date, op->date);
					dist *= 100;
					dist += abs(rawOp->id - op->id);
					if (closestIdx == NONE || dist <= closestDist)
					{
						closestIdx = for_everys_index(op);
						closestDist = dist;
					}
				}
			}
			if (closestIdx != NONE)
			{
				newOperations[closestIdx].b = rawOp->value;
				rawOp->processed = true;
			}
			if (!rawOp->processed)
			{
				error(TXT("what did we sell?"));
				result = false;
			}
		}
	}

	// fee, close
	int maxDists[] = { 1, 5, 10, 30, 80, 100, NONE };
	int maxDistIdx = 0;
	sort(newOperations);
	while (maxDists[maxDistIdx] != NONE)
	{
		int maxDist = maxDists[maxDistIdx];
		bool allOk = false;
		for_every(rawOp, rawOperations)
		{
			if (rawOp->processed)
			{
				continue;

			}
			if (rawOp->type == CryptoOperationRaw::Fee)
			{
				int closestDist = 0;
				int closestIdx = NONE;
				for_every(op, newOperations)
				{
					if (Date::distance_between(op->date, rawOp->date) <= maxDist &&
						!op->cost.is_set() &&
						(op->type == CryptoOperation::Buy || op->type == CryptoOperation::Sell))
					{
						int dist = Date::distance_between(rawOp->date, op->date);
						dist *= 100;
						dist += abs(rawOp->id - op->id);
						if (closestIdx == NONE || dist <= closestDist)
						{
							closestIdx = for_everys_index(op);
							closestDist = dist;
						}
					}
				}
				if (closestIdx != NONE)
				{
					newOperations[closestIdx].cost = rawOp->value;
					rawOp->processed = true;
				}
				if (!rawOp->processed)
				{
					allOk = false;
					if (maxDists[maxDistIdx + 1] == NONE)
					{
						error(TXT("what did we pay for?"));
						result = false;
					}
				}
			}
		}
		++maxDistIdx;
		if (allOk)
		{
			break;
		}
	}

	// sum operations of same type (as bitbay gives them in a bit too much random order :/)
	for (int i = 0; i < newOperations.get_size(); ++i)
	{
		auto & iop = newOperations[i];
		for (int j = i + 1; j < newOperations.get_size(); ++j)
		{
			auto & jop = newOperations[j];
			int dist = Date::distance_between(iop.date, jop.date);
			if (iop.type == jop.type &&
				dist <= 5 &&
				(iop.type == CryptoOperation::Buy ||
					iop.type == CryptoOperation::Sell) &&
				iop.a.name == jop.a.name)
			{
				iop.a.value += jop.a.value;
				iop.a.valuePLN.clear();
				iop.b.access().value += jop.b.get().value;
				iop.b.access().valuePLN.clear();
				iop.cost.access().value += jop.cost.get().value;
				newOperations.remove_at(j);
				--j;
			}
		}
	}

	// move
	operations.add_from(newOperations);

	rawOperations.clear();

	return result;
}

void Crypto::process_queue_buy(IO::File & fOut, CryptoMarkets & markets, CryptoOperation const & op, CryptoCurrencyOperationValue const & a, bool processFee)
{
	CryptoInQueue ciq;
	ciq.amount = a.value;
	ciq.valuePLN = a.valuePLN.get();
	if (processFee)
	{
		ciq.costPLN = op.cost.get().valuePLN.get();
	}
	if (immediateFee && processFee && op.cost.get().name == a.name)
	{
		// just reduce and use it immediately
		ciq.amount -= op.cost.get().value;
		processFee = false;
	}
	markets.markets[op.market].cryptos[a.name].add(fOut, op.market, a.name, ciq);
	if (processFee)
	{
		// take fee (we added cost/fee in PLN to the queue, we just don't want to keep it here)
		if (op.cost.get().name != NAME(PLN))
		{
			Number amount = op.cost.get().value;
			auto & queue = markets.markets[op.market].cryptos[op.cost.get().name];
			while (!queue.is_empty() && amount.is_positive())
			{
				CryptoInQueue provided;
				queue.use(fOut, op.market, op.cost.get().name, REF_ amount, OUT_ provided);
			}
			if (amount.is_positive())
			{
				fOut.write_line(String::printf(TXT("ERROR >> couldn't remove fee (buy) (%S), lack of %S %S"),
					op.date.to_string_universal().to_char(),
					amount.to_string().to_char(), op.cost.get().name.to_char()));
			}
		}
	}
}

void Crypto::process_queue_sell(IO::File & fOut, CryptoMarkets & markets, CryptoOperation const & op, CryptoCurrencyOperationValue const & a, bool processFee,
	Map<int, Number> & overalSold, Map<int, Number> & overalBought, Map<int, Number> & overalFee)
{
	overalSold[op.date.year] += a.valuePLN.get();
	if (processFee && op.cost.is_set())
	{
		overalFee[op.date.year] += op.cost.get().valuePLN.get();
	}
	// take fee first
	if (processFee && op.cost.is_set() && op.cost.get().name != NAME(PLN))
	{
		Number amount = op.cost.get().value;
		auto & queue = markets.markets[op.market].cryptos[op.cost.get().name];
		while (!queue.is_empty() && amount.is_positive())
		{
			CryptoInQueue provided;
			queue.use(fOut, op.market, op.cost.get().name, REF_ amount, OUT_ provided);
		}
		if (amount.is_positive())
		{
			fOut.write_line(String::printf(TXT("ERROR >> couldn't remove fee (sell) (%S), lack of %S %S"),
				op.date.to_string_universal().to_char(),
				amount.to_string().to_char(), op.cost.get().name.to_char()));
		}
	}
	Number amount = a.value;
	auto & queue = markets.markets[op.market].cryptos[a.name];
	while (!queue.is_empty() && amount.is_positive())
	{
		CryptoInQueue provided;
		queue.use(fOut, op.market, a.name, REF_ amount, OUT_ provided);
		overalBought[op.date.year] += provided.valuePLN;
		if (processFee)
		{
			overalFee[op.date.year] += provided.costPLN;
		}
	}
	if (amount.is_positive())
	{
		fOut.write_line(String::printf(TXT("ERROR >> couldn't sell (%S), lack of %S %S"),
			op.date.to_string_universal().to_char(),
			amount.to_string().to_char(), a.name.to_char()));
	}
}

Number minFeePt;
Number maxFeePt;

void output_op(IO::File & fOut, CryptoOperation* op)
{
	if (op->type == CryptoOperation::Buy ||
		op->type == CryptoOperation::Sell)
	{
		tchar const * opType;
		if (op->type == CryptoOperation::Buy)
		{
			opType = TXT("buy");
		}
		else
		{
			opType = TXT("sel");
		}
		Number feePt;
		if (op->cost.is_set())
		{
			feePt = ((op->cost.get().valuePLN.get() / op->a.valuePLN.get()) * Number::integer(100));
		}
		String str;
		str += String::printf(TXT("%S [%10s] %S - %14s %4s"),
			opType, op->market.to_char(),
			op->date.to_string_universal().to_char(),
			op->a.value.to_string(8).to_char(), op->a.name.to_char());
		if (op->b.is_set())
		{
			str += String::printf(TXT(" for %14s %4s"),
				op->b.get().value.to_string().to_char(), op->b.get().name.to_char());
		}
		str += String::printf(TXT("; value %14s PLN"),
			op->a.valuePLN.get().to_string(2).to_char());
		str += String::printf(TXT("; price %9s PLN"),
			(op->a.valuePLN.get() / op->a.value).to_string(2).to_char());
		if (op->cost.is_set())
		{
			str += String::printf(TXT(" (fee %14s %4s; %6s PLN; %S%%%S)"),
				op->cost.get().value.to_string(8).to_char(), op->cost.get().name.to_char(),
				op->cost.get().valuePLN.get().to_string(2).to_char(), feePt.to_string(2).to_char(),
				feePt <= minFeePt ? TXT(" LOW") : (maxFeePt < feePt ? TXT(" HI") : TXT("")));
		}
		fOut.write_line(str);
		if (op->cost.is_set() && (feePt <= minFeePt || maxFeePt < feePt)) // allow zero in some particular cases
		{
			fOut.write_line(TXT("   check fee!"));
		}
	}
	if (op->type == CryptoOperation::InfoTransferIn ||
		op->type == CryptoOperation::InfoTransferOut)
	{
		tchar const * opType;
		if (op->type == CryptoOperation::InfoTransferIn)
		{
			opType = TXT("t?i");
		}
		else
		{
			opType = TXT("t?o");
		}
		fOut.write_line(String::printf(TXT("%S [%10s] %S - %14s %4s  is %14s PLN; price %9s PLN"),
			opType, op->market.to_char(),
			op->date.to_string_universal().to_char(),
			op->a.value.to_string(8).to_char(), op->a.name.to_char(),
			op->a.valuePLN.get().to_string(2).to_char(),
			(op->a.valuePLN.get() / op->a.value).to_string(2).to_char()));
		// 08-12-2017 01:14:35;BTC;coinroom;bitbay;0.05;0.0495;
		Number fee = op->cost.is_set() ? op->cost.get().value : Number();
		fOut.write_line(String::printf(TXT("                 %S;%S;%S;%S;%S;%S;"),
			op->date.to_string_dd_mm_yyyy_hh_mm_ss().to_char(),
			op->a.name.to_char(),
			op->type == CryptoOperation::InfoTransferOut ? op->market.to_char() : TXT(" ??? "),
			op->type == CryptoOperation::InfoTransferIn ? op->market.to_char() : TXT(" ??? "),
			(op->a.value + fee).to_string().to_char(),
			op->a.value.to_string().to_char()));
	}
	if (op->type == CryptoOperation::Transfer)
	{
		tchar const * opType = TXT("trn");
		fOut.write_line(String::printf(TXT("%S [%10s] %S - %14s %4s  is %14s PLN; price %9s PLN -> %10s"),
			opType, op->market.to_char(),
			op->date.to_string_universal().to_char(),
			op->a.value.to_string(8).to_char(), op->a.name.to_char(),
			op->a.valuePLN.get().to_string(2).to_char(),
			(op->a.valuePLN.get() / op->a.value).to_string(2).to_char(),
			op->destMarket.to_char()));
	}
	if (op->type == CryptoOperation::Spending)
	{
		tchar const * opType = TXT("spn");
		fOut.write_line(String::printf(TXT("%S [%10s] %S - %14s %4s  is %14s PLN; price %9s PLN"),
			opType, op->market.to_char(),
			op->date.to_string_universal().to_char(),
			op->a.value.to_string(8).to_char(), op->a.name.to_char(),
			op->a.valuePLN.get().to_string(2).to_char(),
			(op->a.valuePLN.get() / op->a.value).to_string(2).to_char()));
	}
	if (op->type == CryptoOperation::ExtraGain)
	{
		tchar const * opType = TXT("exg");
		if (op->a.valuePLN.is_set())
		{
			fOut.write_line(String::printf(TXT("%S [%10s] %S - %14s %4s  is %14s PLN; price %9s PLN"),
				opType, op->market.to_char(),
				op->date.to_string_universal().to_char(),
				op->a.value.to_string(8).to_char(), op->a.name.to_char(),
				op->a.valuePLN.get().to_string(2).to_char(),
				(op->a.valuePLN.get() / op->a.value).to_string(2).to_char()));
		}
		else
		{
			fOut.write_line(String::printf(TXT("%S [%10s] %S - %14s %4s  VALUE UNKNOWN IN PLN"),
				opType, op->market.to_char(),
				op->date.to_string_universal().to_char(),
				op->a.value.to_string(8).to_char(), op->a.name.to_char()));
		}
	}
}

void output_markets(IO::File & fOut, CryptoMarkets & markets)
{
	for_every(market, markets.markets)
	{
		fOut.write_line(String::printf(TXT("> %S"), for_everys_map_key(market).to_char()));
		for_every(currency, market->cryptos)
		{
			fOut.write_line(String::printf(TXT("  %20s %S"), currency->get_amount().to_string(8).to_char(), for_everys_map_key(currency).to_char()));
		}
	}
}

void Crypto::process(String const & _fileOut)
{
	minFeePt.parse(String(TXT("0.02")));
	maxFeePt.parse(String(TXT("0.60")));

	IO::File fOut;
	fOut.create(_fileOut);
	fOut.set_type(IO::InterfaceType::Text);

	sort(operations);

	build_exchange_rates();

	// process obvious values in PLN
	for_every(op, operations)
	{
		if (op->a.name == NAME(PLN))
		{
			op->a.valuePLN = op->a.value;
			if (op->b.is_set())
			{
				op->b.access().valuePLN = op->a.valuePLN;
			}
		}
		if (op->b.is_set() &&
			op->b.get().name == NAME(PLN))
		{
			op->b.access().valuePLN = op->b.get().value;
			op->a.valuePLN = op->b.get().valuePLN;
		}
	}

	// use exchange rates and fill PLNs
	for_every(op, operations)
	{
		update_pln(op->date, op->a, op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending, true);
		if (op->b.is_set())
		{
			update_pln(op->date, op->b.access(), op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending, true);
		}
		if (op->cost.is_set())
		{
			update_pln(op->date, op->cost.access(), op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending, true);
		}
	}

	// if one is filled, fill the other
	for_every(op, operations)
	{
		if (op->a.valuePLN.is_set() &&
			op->b.is_set() && !op->b.get().valuePLN.is_set())
		{
			op->b.access().valuePLN = op->a.valuePLN;
		}
		if (! op->a.valuePLN.is_set() &&
			op->b.is_set() && op->b.get().valuePLN.is_set())
		{
			op->a.valuePLN = op->b.get().valuePLN;
		}
	}

	// use exchange rates and fill PLNs
	for_every(op, operations)
	{
		update_pln(op->date, op->a, op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending);
		if (op->b.is_set())
		{
			update_pln(op->date, op->b.access(), op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending);
		}
		if (op->cost.is_set())
		{
			update_pln(op->date, op->cost.access(), op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending);
		}
	}

	// check costs, maybe we're dealing with other currency that we would like to change to something we know?
	for_every(op, operations)
	{
		if (op->cost.is_set() &&
			! op->cost.get().valuePLN.is_set())
		{
			CryptoCurrencyOperationValue costKnown = op->cost.get();
			if (op->a.name == costKnown.name &&
				op->b.is_set())
			{
				costKnown.name = op->b.get().name;
				costKnown.value = costKnown.value * op->b.get().value / op->a.value;
				update_pln(op->date, costKnown, op->is_any_kind_of_a_transfer() || op->type == CryptoOperation::Spending);
			}
			if (costKnown.valuePLN.is_set())
			{
				op->cost.access().valuePLN = costKnown.valuePLN.get();
			}
			else
			{
				fOut.write_line(String::printf(TXT("ERROR >> couldn't get cost in PLN")));
			}
		}
	}

	Map<int, Number> overalSold;
	Map<int, Number> overalBought;
	Map<int, Number> overalFee;

	RangeInt years = RangeInt::empty;

	CryptoMarkets markets;
	for_every(op, operations)
	{
		if (op->type == CryptoOperation::Buy)
		{
			if (op->a.name != NAME(PLN))
			{
				process_queue_buy(fOut, markets, *op, op->a, true);
			}
			else
			{
				error(TXT("??"));
			}
			if (op->b.is_set() && op->b.get().name != NAME(PLN))
			{
				process_queue_sell(fOut, markets, *op, op->b.get(), false, overalSold, overalBought, overalFee);
			}
		}
		if (op->type == CryptoOperation::Sell)
		{
			// if no cost defined, could be "small exchange"
			bool processFee = op->cost.is_set();
			if (op->b.is_set() && op->b.get().name != NAME(PLN))
			{
				bool processFeeNow = op->cost.is_set() && immediateFee && op->b.get().name == op->cost.get().name;
				process_queue_buy(fOut, markets, *op, op->b.get(), processFeeNow);
				if (processFeeNow)
				{
					processFee = false;
				}
			}
			if (op->a.name != NAME(PLN))
			{
				years.include(op->date.year);
				process_queue_sell(fOut, markets, *op, op->a, processFee, overalSold, overalBought, overalFee);
			}
			else
			{
				error(TXT("??"));
			}
		}
		if (op->type == CryptoOperation::Spending)
		{
			if (op->a.name != NAME(PLN))
			{
				Number amount = op->a.value;
				Number amountOrg = amount;
				auto & queue = markets.markets[op->market].cryptos[op->a.name];
				while (! queue.is_empty() && amount.is_positive())
				{
					CryptoInQueue provided;
					queue.use(fOut, op->market, op->a.name, REF_ amount, OUT_ provided);
					// we don't do anything else here
				}
				if (amount.is_positive())
				{
					fOut.write_line(String::printf(TXT("ERROR >> couldn't spend (%S), lack of %S %S (was %S %S)"),
						op->date.to_string_universal().to_char(),
						amount.to_string().to_char(), op->a.name.to_char(),
						amountOrg.to_string().to_char(), op->a.name.to_char()));
				}
			}
		}
		if (op->type == CryptoOperation::ExtraGain)
		{
			if (op->a.name != NAME(PLN))
			{
				if (op->a.valuePLN.is_set())
				{
					Number amount = op->a.value;
					Number amountOrg = amount;
					auto& queue = markets.markets[op->market].cryptos[op->a.name];
					CryptoInQueue toAdd;
					toAdd.amount = amount;
					toAdd.costPLN = Number(); // zero
					toAdd.valuePLN = op->a.valuePLN.get();
					queue.add(fOut, op->market, op->a.name, toAdd);
				}
				else
				{
					fOut.write_line(String::printf(TXT("ERROR >> couldn't earn (%S), %S %S no valuePLN"),
						op->date.to_string_universal().to_char(),
						op->a.value.to_string().to_char(), op->a.name.to_char()));
				}
			}
		}
		if (op->type == CryptoOperation::Transfer)
		{
			Number amount = op->a.value;
			auto & sourceQueue = markets.markets[op->market].cryptos[op->a.name];
			auto & destQueue = markets.markets[op->destMarket].cryptos[op->a.name];
			while (!sourceQueue.is_empty() && amount.is_positive())
			{
				CryptoInQueue provided;
				sourceQueue.use(fOut, op->market, op->a.name, REF_ amount, OUT_ provided);
				destQueue.add(fOut, op->destMarket, op->a.name, provided);
			}
			if (amount.is_positive())
			{
				fOut.write_line(String::printf(TXT("ERROR >> couldn't transfer %S to %S (%S), lack of %S %S (of %S)"),
					op->market.to_char(), op->destMarket.to_char(),
					op->date.to_string_universal().to_char(),
					amount.to_string().to_char(), op->a.name.to_char(),
					op->a.value.to_string().to_char()));
			}
		}

		//output_op(fOut, op);
		//output_markets(fOut, markets);
	}
	
	fOut.write_line();
	fOut.write_line();
	fOut.write_line(TXT("operations"));
	fOut.write_line();

	if (!years.is_empty())
	{
		for_range(int, year, years.min, years.max)
		{
			Number totalCost = overalBought[year] + overalFee[year];
			fOut.write_line(String::printf(TXT("%4i : sum %10s : gain %10s : cost %10s = %10s + %10s"),
				year,
				(overalSold[year] - totalCost).to_string(2).to_char(),
				overalSold[year].to_string(2).to_char(),
				(totalCost).to_string(2).to_char(),
				overalBought[year].to_string(2).to_char(),
				overalFee[year].to_string(2).to_char()));
		}
	}
	
	fOut.write_line();
	fOut.write_line();
	fOut.write_line(TXT("markets"));
	fOut.write_line();
	output_markets(fOut, markets);

	fOut.write_line();
	fOut.write_line();
	fOut.write_line(TXT("operations"));
	fOut.write_line();

	{
		for_every(op, operations)
		{
			output_op(fOut, op);
		}
	}
}

bool Crypto::build_exchange_rates()
{
	bool result = true;

	exchangeRates.clear();
	for_every(rateUSD, exchangeRatesUSD)
	{
		int closestIdx = NONE;
		int closestDist = 0;
		for_every(ratePLN, exchangeRatesPLN)
		{
			int dist = Date::difference_between(rateUSD->date, ratePLN->date);
			if (dist >= -60 && (closestIdx == NONE || dist < closestDist))
			{
				closestIdx = for_everys_index(ratePLN);
				closestDist = dist;
			}
		}
		if (closestIdx != NONE)
		{
			auto & ratePLN = exchangeRatesPLN[closestIdx];
			CryptoExchangeRate ex;
			ex.date = rateUSD->date;
			ex.crypto = rateUSD->crypto;
			ex.pricePLN = rateUSD->priceUSD * ratePLN.pricePLN;
			exchangeRates.push_back(ex);
		}
		else
		{
			error(TXT("no exchange rate for USD on %S!"),rateUSD->date.to_string_universal().to_char());
		}
	}

	for_every(op, operations)
	{
		if (op->type == CryptoOperation::Buy ||
			op->type == CryptoOperation::Sell)
		{
			if (op->b.is_set() &&
				op->b.get().name == NAME(PLN))
			{
				CryptoExchangeRate ex;
				ex.date = op->date;
				ex.crypto = op->a.name;
				ex.pricePLN = op->b.get().value / op->a.value;
				an_assert(ex.pricePLN.value >= 0);
				ex.pricePLN.limit_point_at_to(8);
				exchangeRates.push_back(ex);
			}
		}
	}

	return result;
}

void Crypto::update_pln(Date const & _date, CryptoCurrencyOperationValue & _value, bool _noWarning, bool _onlyClose) const
{
	if (!_value.valuePLN.is_set())
	{
		if (_value.name == NAME(PLN))
		{
			_value.valuePLN = _value.value;
			return;
		}
		int closestIdx = NONE;
		int closestDist = 0;
		for_every(rate, exchangeRates)
		{
			if (rate->crypto == _value.name)
			{
				int dist = Date::difference_between(_date, rate->date);
				if (dist >= -60 && (closestIdx == NONE || dist < closestDist))
				{
					closestIdx = for_everys_index(rate);
					closestDist = dist;
				}
			}
		}

		if (closestIdx != NONE)
		{
			if (_onlyClose && closestDist > 86400)
			{
				return;
			}

			if (closestDist > 86400)
			{
				if (!_noWarning)
				{
					warn(TXT("find rate within a day! %S %S"), _value.name.to_char(), _date.to_string_universal().to_char());
				}
			}
			auto const & rate = exchangeRates[closestIdx];
			_value.valuePLN = _value.value * rate.pricePLN;
		}
	}
}
