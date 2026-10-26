class Coinconvert < Formula
  desc "Convert cryptocurrency and fiat amounts using live exchange rates"
  homepage "https://github.com/coinconvert/coinconvert"
  license "MIT"
  version "1.0.0"

  # Source and bottle live in this repository. A local tap makes both file
  # URLs point at the tap checkout, so `brew install` does not need a forge.
  url "file://#{File.expand_path("../bottles/coinconvert-1.0.0.tar.gz", __dir__)}"
  sha256 "6105e4a94a3a54008e99d52ed1102f6592186addb9b2363158b0d3e274fe4994"

  bottle do
    root_url "file://#{File.expand_path("../bottles", __dir__)}"
    sha256 cellar: :any_skip_relocation, arm64_golden_gate: "7b4336bc6cae2972d153d264916c6017bfa33b15734a0aafd3585219ac8bb918"
  end

  # Source builds need a toolchain Homebrew accepts for this macOS version.
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
