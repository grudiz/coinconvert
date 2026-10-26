class Coinconvert < Formula
  desc "Convert cryptocurrency and fiat amounts using live exchange rates"
  homepage "https://github.com/grudiz/coinconvert"
  url "https://github.com/grudiz/coinconvert/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "50981c4a26eeb0be49851fcd24c0544a6a6b504e4c0e0084487a843210c0f351"
  license "MIT"
  head "https://github.com/grudiz/coinconvert.git", branch: "main"

  uses_from_macos "curl"

  def install
    system "make", "install", "PREFIX=#{prefix}"
  end

  test do
    assert_equal <<~EOS, shell_output("#{bin}/coinconvert xmr 1 xmr")
      1 XMR = 1.00 XMR
      provider: identity
    EOS
    assert_match "unknown currency", shell_output("#{bin}/coinconvert nope 1 usd 2>&1", 1)
  end
end
