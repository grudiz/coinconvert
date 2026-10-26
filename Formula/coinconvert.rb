class Coinconvert < Formula
  desc "Convert cryptocurrency and fiat amounts using live exchange rates"
  homepage "https://github.com/grudiz/coinconvert"
  url "file://#{File.expand_path("../bottles/coinconvert-1.0.0.tar.gz", __dir__)}"
  sha256 "e2cd0097b05cffa7bc5cea1c7b6984acc0315dc88ecc0f2ef5f8aa95771a7af0"
  license "MIT"

  # Local tap only. homebrew-core bottles are added by BrewTestBot and must
  # not use a file:// root_url.
  head "file://#{File.expand_path("..", __dir__)}", using: :git, branch: "main"

  bottle do
    root_url "file://#{File.expand_path("../bottles", __dir__)}"
    sha256 cellar: :any_skip_relocation, arm64_golden_gate: "7b4336bc6cae2972d153d264916c6017bfa33b15734a0aafd3585219ac8bb918"
  end

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
