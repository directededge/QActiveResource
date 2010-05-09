#!/usr/bin/ruby

require 'rubygems'
require 'active_resource'

type = ARGV.shift || 'REXML'
count = ARGV.shift || 100

case type
when 'nokogiri'
  require 'nokogiri'
  ActiveSupport::XmlMini.backend = 'Nokogiri'
when 'qar'
  require 'QAR'
  extend = 'extend QAR'
end

resource = ENV['AR_RESOURCE'].capitalize

eval "class #{resource} < ActiveResource::Base ; #{extend} ; self.site = ENV['AR_BASE'] ; end"

resource = Kernel.const_get(resource)
field = ENV['AR_FIELD']

unless type == 'dummy'
  (1..count).each { |i| puts i ; resource.find(:all).each { |p| puts p.send(field) } }
end
