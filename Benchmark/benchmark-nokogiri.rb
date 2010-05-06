#!/usr/bin/ruby

require 'rubygems'
require 'active_resource'
require 'nokogiri'

ActiveSupport::XmlMini.backend='Nokogiri'

resource = ENV['AR_RESOURCE'].capitalize

eval "class #{resource} < ActiveResource::Base ; self.site = ENV['AR_BASE'] ; end"

resource = Kernel.const_get(resource)
field = ENV['AR_FIELD']

(1..100).each { |i| puts i ; resource.find(:all).each { |p| puts p.send(field) } }

