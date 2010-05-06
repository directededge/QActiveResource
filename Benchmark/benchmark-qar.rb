#!/usr/bin/ruby

require 'QAR'
require 'pp'

resource = QAR::Resource.new(ENV['AR_BASE'], ENV['AR_RESOURCE'])
field = ENV['AR_FIELD']
(1..100).each { |i| puts i ; resource.find.each { |r| puts r[field] } }
