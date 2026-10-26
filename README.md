# coinconvert

`coinconvert` converts an amount between cryptocurrencies and fiat currencies, prints the result, and exits. It reads one live rate from a provider you choose. It does not open a prompt or a window, and it does not fill in a price when that provider has no rate.

```text
coinconvert xmr 0.004 eur
0.004 XMR = 2.09 EUR
provider: coingecko
```

The number on the right is rounded to two decimal places. The second line is the provider that supplied the rate.

## Install with Homebrew

From the repository root, on the `main` branch, with a clean work tree:

```sh
./scripts/install-homebrew.sh
```

The script taps this checkout as `coinconvert/coinconvert` and installs the formula. Apple silicon macOS 27 installs the bottled binary in `bottles/`, which does not compile. After that, `coinconvert` is on your `PATH`.

The same steps by hand:

```sh
brew tap coinconvert/coinconvert "$(pwd)"
HOMEBREW_NO_AUTO_UPDATE=1 brew install --formula coinconvert/coinconvert/coinconvert
```

`brew install --HEAD --formula coinconvert/coinconvert/coinconvert` builds `main` from source. That path needs the Xcode version Homebrew requires for this macOS release. `make` in this repository uses the system compiler directly and does not apply that check.

Run the script again after new commits. It retaps the checkout and reinstalls.

The formula's test runs `coinconvert --list-providers`, `--list-currencies`, and `--version`. Homebrew still checks the Xcode version before `brew test`, so on a machine it refuses to compile for, run those three commands directly. `make test` covers the parser, the crosses, and the command output without the network.

## Build from source

macOS already has `clang` and `libcurl`. A C++17 compiler and libcurl headers are enough on other systems (`build-essential` and `libcurl4-openssl-dev` on Debian or Ubuntu).

```sh
make
make test
./build/coinconvert --help
```

Install the binary somewhere else:

```sh
make install PREFIX="$HOME/.local"
```

## Usage

```text
coinconvert [--provider NAME] FROM AMOUNT TO
coinconvert --list-providers
coinconvert --list-currencies
coinconvert --help
coinconvert --version
```

`FROM` and `TO` are currency codes. The amount is a non-negative number (`0.004`, `10`, `1e-3`). Codes and provider names are case-insensitive.

```sh
coinconvert xmr 0.004 eur
coinconvert --provider kraken btc 0.01 usd
coinconvert eth 2 xmr
coinconvert eur 25 gbp
coinconvert -p coinpaprika usd 100 jpy
```

Exit status `0` means a rate was printed. Exit status `1` means the arguments were wrong. Exit status `2` means the provider had no rate or the request failed.

Converting a code to itself does not call a provider. The second line is `provider: identity`, and the result is still rounded to two decimal places, so `0.004 XMR` displayed as XMR is `0.00`.

## Providers

| `--provider` | What it reads |
| --- | --- |
| `coingecko` (default) | CoinGecko `simple/price` |
| `coinpaprika` | CoinPaprika `price-converter` |
| `kraken` | Kraken public ticker, last trade |
| `bitfinex` | Bitfinex public ticker, last trade |

Pick a provider when you want that venue's price. The default does not try the next provider if the first one fails. CoinGecko's free endpoint rate-limits repeated calls; the command exits with the provider's error instead of switching providers on its own.

A direct book is used when that provider publishes one. Kraken and Bitfinex otherwise cross two of their own books through USD, EUR, GBP, BTC, or ETH, in that order. CoinGecko prices a crypto in the target when that target is one of its quote currencies, and otherwise divides two of its USD prices. Fiat-to-fiat on CoinGecko is the ratio of CoinGecko's BTC price in the two currencies. Every number in the cross comes from the same response, or from the same provider on one request. Providers are never mixed, and a missing field is an error.

These APIs were checked on 2026-10-26 before they were wired in. The core pairs were XMR, BTC, and ETH against EUR, USD, and GBP, plus the other codes in `--list-currencies` where the provider actually lists them.

Not included, because they did not cover those pairs with a live book:

- Coinbase has no XMR market.
- Binance lists `XMRUSDT` and `BTCGBP` as `BREAK`, with an empty book. The leftover last price does not match the live XMR market.
- CryptoCompare's public price API requires a key.
- CoinCap's host did not resolve.
- Gate.io rejects XMR. OKX has no XMR instrument.
- KuCoin has XMR, BTC, and ETH against USDT or EUR, and no USD or GBP book for them.
- MEXC has XMR against USDT and BTC or ETH against EUR, and no GBP book.

Stablecoins such as USDT are currencies you can name. They are not treated as USD inside a cross.

## Currencies

`--list-currencies` prints the codes this build knows. Crypto rows carry a CoinGecko id that returned a USD price, and a CoinPaprika id when their converter accepted it. Fiat rows are quote currencies CoinGecko publishes. PHP, AED, and SAR are in that fiat set and are absent from CoinPaprika, so `--provider coinpaprika` reports that it has no rate for them.

Kraken and Bitfinex only price codes that appear in their market tables. Anything else returns `has no rate` without inventing a path through another exchange.

## Adding a currency

1. Confirm the id. For CoinGecko, `simple/price` must return a positive number. For CoinPaprika, `price-converter` must return a positive `price`. Leave the Paprika id empty when it does not.
2. Add one row to the table in `src/currency.cpp`.
3. Add a Kraken or Bitfinex market only after that venue's public pair list shows the exact symbol. Put the symbol in `src/kraken.cpp` or `src/bitfinex.cpp`.
4. Run `make test`.

## Adding a provider

1. Call the public API and keep the provider out until the pairs you claim, including XMR against EUR, USD, and GBP where you claim them, come back as real prices.
2. Implement `PriceProvider` in a new pair of files. Return only numbers from that response. Throw `no_rate` when a field is missing, zero, or non-numeric.
3. Register the object in `src/registry.cpp`.
4. Add a fixture in `tests/test_coinconvert.cpp` that feeds a recorded response and checks the rate, and another fixture where an error payload must not become a price.

`make test` covers parsing, rounding, market crosses, and the command output without using the network. A real conversion needs network access at run time.
