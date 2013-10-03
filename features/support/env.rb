require "aruba/cucumber"


Before do
  @dirs = [ File.join( File.dirname( __FILE__ ), "..", ".." ) ]
  @aruba_timeout_seconds = 30
end
