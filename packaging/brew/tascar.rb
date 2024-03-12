class Tascar < Formula
  desc "Toolbox for Acoustic Scene Creation and Rendering"
  homepage "http://tascar.org/"
  url "https://github.com/gisogrimm/tascar/archive/refs/tags/release_0.229.1.tar.gz"
  sha256 "340bfa78d5391e68dbc85022e26aa4397baace06da91a34e7f018007781f2778"
  license "GPL v2"

  depends_on "jack"
  depends_on "fftw"
  depends_on "xerces-c"
  depends_on "eigen"
  depends_on "gsl"
  depends_on "liblo"
  depends_on "libsndfile"
  depends_on "gtkmm3"
  depends_on "libltc"
  depends_on "libmatio"
  depends_on "gtksourceviewmm3"

  def install
    system "make"
    system "make", "install"
  end

  test do
    system "make", "test"
  end
end
