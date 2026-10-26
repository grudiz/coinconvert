class Coinconvert < Formula
  desc "Convert cryptocurrency and fiat amounts using live exchange rates"
  homepage "https://github.com/coinconvert/coinconvert"
  license "MIT"
  # The formula lives in this repository. --HEAD clones the tap checkout,
  # which is the project itself when installed from a local tap.
  head "file://#{File.expand_path("..", __dir__)}", using: :git, branch: "main"

  def install
    system "make"
    bin.install "build/coinconvert"
  end

  test do
    assert_match "coingecko", shell_output("#{bin}/coinconvert --list-providers")
    assert_match "XMR", shell_output("#{bin}/coinconvert --list-currencies")
    assert_match "1.0.0", shell_output("#{bin}/coinconvert --version")
  end
end
