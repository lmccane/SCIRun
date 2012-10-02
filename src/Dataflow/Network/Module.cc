/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <Dataflow/Network/PortManager.h>
#include <Dataflow/Network/ModuleStateInterface.h>
#include <Dataflow/Network/DataflowInterfaces.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Network/NullModuleState.h>
#include <Core/Logging/ConsoleLogger.h>

using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Engine::State;
using namespace SCIRun::Core::Logging;

std::string SCIRun::Dataflow::Networks::to_string(const ModuleInfoProvider& m)
{
  return m.get_module_name() + " [" + m.get_id() + "]";
}

/*static*/ int Module::instanceCount_ = 0;
/*static*/ LoggerHandle Module::defaultLogger_(new ConsoleLogger);

Module::Module(const ModuleLookupInfo& info,
  ModuleStateFactoryHandle stateFactory,
  bool hasUi,
  const std::string& version)
  : info_(info), has_ui_(hasUi), executionTime_(1.0), state_(stateFactory ? stateFactory->make_state(info.module_name_) : new NullModuleState)
{
  id_ = info_.module_name_ + boost::lexical_cast<std::string>(instanceCount_++);
  iports_.set_module(this);
  oports_.set_module(this);
  log_ = defaultLogger_;
}

Module::~Module()
{
  instanceCount_--;
}

ModuleStateFactoryHandle Module::defaultStateFactory_;

OutputPortHandle Module::get_output_port(size_t idx) const
{
  return oports_[idx];
}

InputPortHandle Module::get_input_port(size_t idx) const
{
  return iports_[idx];
}

size_t Module::num_input_ports() const
{
  return iports_.size();
}

size_t Module::num_output_ports() const
{
  return oports_.size();
}

void Module::do_execute()
{
  log_->status("STARTING MODULE: "+id_);

  // Reset all of the ports.
  oports_.resetAll();
  iports_.resetAll();

  try 
  {
    execute();
  }
  catch(const std::bad_alloc&)
  {
    log_->error("MODULE ERROR: bad_alloc caught");
  }
  catch (Core::ExceptionBase& e)
  {
    std::ostringstream ostr;
    ostr << "Caught exception: " << e.typeName();
    ostr << "\n";
    ostr << "Message: " << e.what() << "\n";
    
    ostr << "TODO! Following error info will be filtered later, it's too technical for general audiences.\n";
    ostr << boost::diagnostic_information(e) << std::endl;
    log_->error(ostr.str());
  }
  catch (const std::exception& e)
  {
    log_->error("MODULE ERROR: std::exception caught");
    log_->error(e.what());
  }
  catch (...)
  {
    log_->error("MODULE ERROR: unhandled exception caught");
  }

  // Call finish on all ports.
  //iports_.apply(boost::bind(&PortInterface::finish, _1));
  //oports_.apply(boost::bind(&PortInterface::finish, _1));

  log_->status("MODULE FINISHED: " + id_);  
}

ModuleStateHandle Module::get_state() 
{
  return state_;
}

void Module::set_state(ModuleStateHandle state) 
{
  state_ = state;
}

void Module::add_input_port(InputPortHandle h)
{
  iports_.add(h);
}

void Module::add_output_port(OutputPortHandle h)
{
  oports_.add(h);
}

SCIRun::Core::Datatypes::DatatypeHandleOption Module::get_input_handle(size_t idx)
{
  //TODO test...
  if (idx >= iports_.size())
  {
    BOOST_THROW_EXCEPTION(PortNotFoundException() << Core::ErrorMessage("Port not found: " + boost::lexical_cast<std::string>(idx)));
  }

  return iports_[idx]->getData();
}

void Module::send_output_handle(size_t idx, SCIRun::Core::Datatypes::DatatypeHandle data)
{
  //TODO test...
  if (idx >= oports_.size())
  {
    throw std::invalid_argument("port does not exist at index " + boost::lexical_cast<std::string>(idx));
  }

  oports_[idx]->sendData(data);
}

Module::Builder::Builder() 
{
}

Module::Builder::SinkMaker Module::Builder::sink_maker_;
Module::Builder::SourceMaker Module::Builder::source_maker_;

/*static*/ void Module::Builder::use_sink_type(Module::Builder::SinkMaker func) { sink_maker_ = func; }
/*static*/ void Module::Builder::use_source_type(Module::Builder::SourceMaker func) { source_maker_ = func; }

class DummyModule : public Module
{
public:
  explicit DummyModule(const ModuleLookupInfo& info) : Module(info) {}
  virtual void execute() 
  {
    std::cout << "Module " << get_module_name() << " executing for " << executionTime_ << " seconds." << std::endl;
  }
};

Module::Builder& Module::Builder::with_name(const std::string& name)
{
  if (!module_)
  {
    ModuleLookupInfo info;
    info.module_name_ = name;
    module_.reset(new DummyModule(info));
  }
  return *this;
}

Module::Builder& Module::Builder::using_func(ModuleMaker create)
{
  if (!module_)
    module_.reset(create());
  return *this;
}

Module::Builder& Module::Builder::add_input_port(const Port::ConstructionParams& params)
{
  if (module_)
  {
    DatatypeSinkInterfaceHandle sink(sink_maker_ ? sink_maker_() : 0);
    InputPortHandle port(new InputPort(module_.get(), params, sink));
    module_->add_input_port(port);
  }
  return *this;
}

Module::Builder& Module::Builder::add_output_port(const Port::ConstructionParams& params)
{
  if (module_)
  {
    DatatypeSourceInterfaceHandle source(source_maker_ ? source_maker_() : 0);
    OutputPortHandle port(new OutputPort(module_.get(), params, source));
    module_->add_output_port(port);
  }
  return *this;
}

Module::Builder& Module::Builder::disable_ui()
{
  module_->has_ui_ = false;
  return *this;
}

ModuleHandle Module::Builder::build()
{
  return module_;
}
