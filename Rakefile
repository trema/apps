require "cucumber/rake/task"


Cucumber::Rake::Task.new


def apps
  ( Dir.glob( "*" ) - [ "features" ] ).select do | each |
    File.directory?( each )
  end
end


def c_apps
  apps.select do | each |
    FileTest.exists? File.join( each, "Makefile" )
  end
end


task :clean do
  c_apps.each do | each |
    cd each do
      sh "make clean"
    end
  end
end
