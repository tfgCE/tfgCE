#pragma once

#include "date.h"
#include "number.h"

#include "..\core\containers\array.h"
#include "..\core\containers\list.h"
#include "..\core\containers\map.h"
#include "..\core\types\name.h"

namespace Budget
{
	struct CryptoCurrency
	{
		Name name;
		Number amount;
	};

	struct CryptoCurrencyOperationValue
	{
		Name name;
		Number value;
		Optional<Number> valuePLN;
	};

	struct CryptoExchangeRate
	{
		Date date;
		Name crypto; // or USD :D
		Number pricePLN;
	};

	struct CryptoExchangeRateUSD
	{
		Date date;
		Name crypto;
		Number priceUSD;
	};

	struct CryptoOperation
	{
		enum Type
		{
			Buy, // buy (a) with fiat (b)
			Sell, // sell (a) with fiat (b)
			Exchange, // crypto-crypto
			// to keep track of the amount
			Transfer, // just to keep amount
			Spending, // just to keep amount
			ExtraGain, // extra amount gained
			InfoTransferOut, // actual transfers come from transfer file
			InfoTransferIn, // actual transfers come from transfer file
		};

		bool is_any_kind_of_a_transfer() const
		{
			return type == Transfer || type == InfoTransferIn || type == InfoTransferOut;
		}

		Date date;
		int id = 0;
		Type type;
		Name market;
		Name destMarket;

		CryptoCurrencyOperationValue a;
		Optional<CryptoCurrencyOperationValue> b;

		Optional<CryptoCurrencyOperationValue> cost; //

		static int compare(void const * _a, void const * _b)
		{
			CryptoOperation const * a = plain_cast<CryptoOperation>(_a);
			CryptoOperation const * b = plain_cast<CryptoOperation>(_b);
			return a->date == b->date ? (a->id == b->id ? A_AS_B : (a->id < b->id ? A_BEFORE_B : B_BEFORE_A)) : (a->date < b->date ? A_BEFORE_B : B_BEFORE_A);
		}
	};

	struct CryptoOperationRaw
	{
		enum Type
		{
			In, // exchange currency
			Out, // exchange currency
			BuyIn, // bought currency
			BuyOut, // bought currency with other currency
			SellOut, // sold currency
			SellIn, // got this in exchange for sold currency
			ExtraGain, // any extra gains, mining, offers cancelled
			Fee, // fee/cost
			// transfers only
			InfoTransferOut,
			InfoTransferIn,
		};

		Date date;
		int id = 0;
		Type type;
		Name market;

		bool processed = false;

		CryptoCurrencyOperationValue value;

		static int compare(void const * _a, void const * _b)
		{
			CryptoOperationRaw const * a = plain_cast<CryptoOperationRaw>(_a);
			CryptoOperationRaw const * b = plain_cast<CryptoOperationRaw>(_b);
			return a->date == b->date? (a->id == b->id? A_AS_B : (a->id < b->id? A_BEFORE_B : B_BEFORE_A)) : (a->date < b->date? A_BEFORE_B : B_BEFORE_A);
		}
	};

	struct CryptoInQueue
	{
		Number amount; // + buy, - sell
		Number valuePLN; // value that goes for TAX, if this is fee for transfer, just drop it - be zero
		Number costPLN; 
	};

	struct CryptoQueue
	{
		List<CryptoInQueue> queue;
		bool is_empty() const { return queue.is_empty(); }
		void add(IO::File & fOut, Name const & _market, Name const & _crypto, CryptoInQueue const & _provided);
		void use(IO::File & fOut, Name const & _market, Name const & _crypto, REF_ Number & _amount, OUT_ CryptoInQueue & _provided);
		Number get_amount() const;
	};

	struct CryptoMarket
	{
		Map<Name, CryptoQueue> cryptos;
	};

	struct CryptoMarkets
	{
		Map<Name, CryptoMarket> markets;
	};

	struct Crypto
	{
		int id = 0;

		Array<CryptoCurrency> currencies; // how many do we have right now
		Array<CryptoOperation> operations; // processed
		Array<CryptoOperationRaw> rawOperations; // as loaded
		Array<CryptoExchangeRate> exchangeRates; // temp, will be removed and rebuilt
		Array<CryptoExchangeRate> exchangeRatesPLN;
		Array<CryptoExchangeRateUSD> exchangeRatesUSD;

		bool load_dir(String const & _dir);

		bool load_binance_csv(IO::File* _file);
		bool load_binance_2_csv(IO::File* _file);
		bool load_bitbay_csv(IO::File* _file);
		bool load_coinroom_csv(IO::File* _file);
		bool load_pln_usd_rates_csv(IO::File* _file);
		bool load_usd_rates_csv(IO::File* _file, String const & _currency);
		bool load_transfer_csv(IO::File* _file);
		bool load_extra_gain_csv(IO::File* _file);

		void process(String const & _fileOut);

	private:
		bool build_exchange_rates();
		bool process_raw_operations_bitbay();
		bool process_raw_operations_binance_2();

		void update_pln(Date const & _date, CryptoCurrencyOperationValue & _value, bool _noWarning = false, bool _onlyClose = false) const;

		void process_queue_buy(IO::File & fOut, CryptoMarkets & markets, CryptoOperation const & op, CryptoCurrencyOperationValue const & a, bool processFee);
		void process_queue_sell(IO::File & fOut, CryptoMarkets & markets, CryptoOperation const & op, CryptoCurrencyOperationValue const & a, bool processFee,
			Map<int, Number> & overalSold, Map<int, Number> & overalBought, Map<int, Number> & overalFee);
	};
};

void process_crypto(String const & _dirInput, String const & _fileOut);

