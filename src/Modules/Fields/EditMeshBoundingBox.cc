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

#include <Modules/Fields/EditMeshBoundingBox.h>
#include <Core/Datatypes/Legacy/Field/Field.h>
#include <Core/Datatypes/Legacy/Field/VMesh.h>

using namespace SCIRun;
using namespace SCIRun::Modules::Fields;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Geometry;

const ModuleLookupInfo EditMeshBoundingBox::staticInfo_("EditMeshBoundingBox", "ChangeMesh", "SCIRun");

namespace SCIRun
{
  class SCISHARE GeometryPortInterface
  {

  };

  class SCISHARE BoxWidgetInterface
  {
  public:
    virtual ~BoxWidgetInterface() {}
    //TODO: need conversion to GPI type above, maybe
    virtual void connect(OutputPortHandle port) = 0;
    virtual void setRestrictX(bool restrict) = 0;
    virtual void setRestrictY(bool restrict) = 0;
    virtual void setRestrictZ(bool restrict) = 0;
    virtual void setRestrictR(bool restrict) = 0;
    virtual void setRestrictD(bool restrict) = 0;
    virtual void setRestrictI(bool restrict) = 0;
    virtual void unrestrictTranslation() = 0;
    virtual void restrictTranslationXYZ() = 0;
    virtual void restrictTranslationRDI() = 0;
  };
}

class BoxWidgetNull : public BoxWidgetInterface
{
public:
  virtual void connect(OutputPortHandle port) override
  {
    std::cout << "BoxWidgetNull::connect called" << std::endl;
  }
  virtual void setRestrictX(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictX called with " << restrict << std::endl;
  }
  virtual void setRestrictY(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictY called with " << restrict << std::endl;
  }
  virtual void setRestrictZ(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictZ called with " << restrict << std::endl;
  }
  virtual void setRestrictR(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictR called with " << restrict << std::endl;
  }
  virtual void setRestrictD(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictD called with " << restrict << std::endl;
  }
  virtual void setRestrictI(bool restrict) override
  {
    std::cout << "BoxWidgetNull::setRestrictI called with " << restrict << std::endl;
  }
  virtual void unrestrictTranslation() override
  {
    std::cout << "BoxWidgetNull::unrestrictTranslation called" << std::endl;
  }
  virtual void restrictTranslationXYZ() override
  {
    std::cout << "BoxWidgetNull::restrictTranslationXYZ called" << std::endl;
  }
  virtual void restrictTranslationRDI() override
  {
    std::cout << "BoxWidgetNull::restrictTranslationRDI called" << std::endl;
  }
};

class WidgetFactory
{
public:
  static BoxWidgetPtr createBox();
};

EditMeshBoundingBox::EditMeshBoundingBox() : Module(staticInfo_)
{
  INITIALIZE_PORT(InputField);
  INITIALIZE_PORT(OutputField);
  INITIALIZE_PORT(Transformation_Widget);
  INITIALIZE_PORT(Transformation_Matrix);
}

void EditMeshBoundingBox::createBoxWidget()
{
  box_ = WidgetFactory::createBox();  //new BoxWidget(this, &widget_lock_, 1.0, false, false);
  box_->connect(getOutputPort(Transformation_Widget));
}

void EditMeshBoundingBox::setStateDefaults()
{
  clear_vals();
  auto state = get_state();
  state->setValue(RestrictX, false);
  state->setValue(RestrictY, false);
  state->setValue(RestrictZ, false);
  state->setValue(RestrictR, false);
  state->setValue(RestrictD, false);
  state->setValue(RestrictI, false);
  //TODO

  createBoxWidget();
  setBoxRestrictions();
}

void EditMeshBoundingBox::execute()
{
  //TODO: need version to pass a func for ifNull case--fancy but useful. For now just reset each time.
  clear_vals();
  setBoxRestrictions();
  auto field = getRequiredInput(InputField);

  if (needToExecute())
  {
    update_state(Executing);
    update_input_attributes(field);
  }
}

void EditMeshBoundingBox::clear_vals()
{
  auto state = get_state();
  const std::string cleared("---");
  state->setValue(InputCenterX, cleared);
  state->setValue(InputCenterY, cleared);
  state->setValue(InputCenterZ, cleared);
  state->setValue(InputSizeX, cleared);
  state->setValue(InputSizeY, cleared);
  state->setValue(InputSizeZ, cleared);
}

void EditMeshBoundingBox::update_input_attributes(FieldHandle f)
{
  Point center;
  Vector size;

  BBox bbox = f->vmesh()->get_bounding_box();

  if (!bbox.valid())
  {
    warning("Input field is empty -- using unit cube.");
    bbox.extend(Point(0, 0, 0));
    bbox.extend(Point(1, 1, 1));
  }
  size = bbox.diagonal();
  center = bbox.center();

  auto state = get_state();
  state->setValue(InputCenterX, boost::lexical_cast<std::string>(center.x()));
  state->setValue(InputCenterY, boost::lexical_cast<std::string>(center.y()));
  state->setValue(InputCenterZ, boost::lexical_cast<std::string>(center.z()));
  state->setValue(InputSizeX, boost::lexical_cast<std::string>(size.x()));
  state->setValue(InputSizeY, boost::lexical_cast<std::string>(size.y()));
  state->setValue(InputSizeZ, boost::lexical_cast<std::string>(size.z()));
}


void
EditMeshBoundingBox::build_widget(FieldHandle f, bool reset)
{
#ifdef WORKING_ON_EDITMESH_DAN
  if (reset || box_scale_.get() <= 0 ||
    (box_center_.get() == Point(0.0, 0.0, 0.0) &&
    box_right_.get() == Point(0.0, 0.0, 0.0) &&
    box_down_.get() == Point(0.0, 0.0, 0.0) &&
    box_in_.get() == Point(0.0, 0.0, 0.0)))
  {
    Point center;
    Vector size;
    BBox bbox = f->vmesh()->get_bounding_box();
    if (!bbox.valid()) {
      warning("Input field is empty -- using unit cube.");
      bbox.extend(Point(0, 0, 0));
      bbox.extend(Point(1, 1, 1));
    }
    box_initial_bounds_ = bbox;

    // build a widget identical to the BBox
    size = Vector(bbox.max() - bbox.min());
    if (fabs(size.x())<1.e-4)
    {
      size.x(2.e-4);
      bbox.extend(bbox.min() - Vector(1.0e-4, 0.0, 0.0));
      bbox.extend(bbox.max() + Vector(1.0e-4, 0.0, 0.0));
    }
    if (fabs(size.y())<1.e-4)
    {
      size.y(2.e-4);
      bbox.extend(bbox.min() - Vector(0.0, 1.0e-4, 0.0));
      bbox.extend(bbox.max() + Vector(0.0, 1.0e-4, 0.0));
    }
    if (fabs(size.z())<1.e-4)
    {
      size.z(2.e-4);
      bbox.extend(bbox.min() - Vector(0.0, 0.0, 1.0e-4));
      bbox.extend(bbox.max() + Vector(0.0, 0.0, 1.0e-4));
    }
    center = Point(bbox.min() + size / 2.);

    Vector sizex(size.x(), 0, 0);
    Vector sizey(0, size.y(), 0);
    Vector sizez(0, 0, size.z());

    Point right(center + sizex / 2.);
    Point down(center + sizey / 2.);
    Point in(center + sizez / 2.);

    // Translate * Rotate * Scale.
    Transform r;
    box_initial_transform_.load_identity();
    box_initial_transform_.pre_scale(Vector((right - center).length(),
      (down - center).length(),
      (in - center).length()));
    r.load_frame((right - center).safe_normal(),
      (down - center).safe_normal(),
      (in - center).safe_normal());
    box_initial_transform_.pre_trans(r);
    box_initial_transform_.pre_translate(center.asVector());

    const double newscale = size.length() * 0.015;
    double bscale = box_real_scale_.get();
    if (bscale < newscale * 1e-2 || bscale > newscale * 1e2)
    {
      bscale = newscale;
    }
    box_->SetScale(bscale); // callback sets box_scale for us.
    box_->SetPosition(center, right, down, in);
    box_->SetCurrentMode(box_mode_.get());
    box_center_.set(center);
    box_right_.set(right);
    box_down_.set(down);
    box_in_.set(in);
    box_scale_.set(-1.0);
  }
  else
  {

    const double l2norm = (box_right_.get().vector() +
      box_down_.get().vector() +
      box_in_.get().vector()).length();
    const double newscale = l2norm * 0.015;
    double bscale = box_real_scale_.get();
    if (bscale < newscale * 1e-2 || bscale > newscale * 1e2)
    {
      bscale = newscale;
    }
    box_->SetScale(bscale); // callback sets box_scale for us.
    box_->SetPosition(box_center_.get(), box_right_.get(),
      box_down_.get(), box_in_.get());
    box_->SetCurrentMode(box_mode_.get());
  }

  GeomGroup *widget_group = new GeomGroup;
  widget_group->add(box_->GetWidget());

  GeometryOPortHandle ogport;
  get_oport_handle("Transformation Widget", ogport);
  widgetid_ = ogport->addObj(widget_group, "EditMeshBoundingBox Transform widget", &widget_lock_);
  ogport->flushViews();
#endif
}

void EditMeshBoundingBox::setBoxRestrictions()
{
  auto state = get_state();
  box_->setRestrictX(state->getValue(RestrictX).toBool());
  box_->setRestrictY(state->getValue(RestrictY).toBool());
  box_->setRestrictZ(state->getValue(RestrictZ).toBool());
  box_->setRestrictR(state->getValue(RestrictR).toBool());
  box_->setRestrictD(state->getValue(RestrictD).toBool());
  box_->setRestrictI(state->getValue(RestrictI).toBool());
  if (state->getValue(NoTranslation).toBool())
    box_->unrestrictTranslation();
  else if (state->getValue(XYZTranslation).toBool())
    box_->restrictTranslationXYZ();
  else if (state->getValue(RDITranslation).toBool())
    box_->restrictTranslationRDI();
}

void EditMeshBoundingBox::executeImpl()
{
#ifdef WORKING_ON_EDITMESH_DAN
  // build the transform widget and set the the initial
  // field transform.

  if true
  {
    build_widget(fh, resetting_.get());
    BBox bbox = fh->vmesh()->get_bounding_box();
    if (!bbox.valid()) {
      warning("Input field is empty -- using unit cube.");
      bbox.extend(Point(0, 0, 0));
      bbox.extend(Point(1, 1, 1));
    }
    Vector size(bbox.max() - bbox.min());
    if (fabs(size.x())<1.e-4)
    {
      size.x(2.e-4);
      bbox.extend(bbox.min() - Vector(1.e-4, 0, 0));
    }
    if (fabs(size.y())<1.e-4)
    {
      size.y(2.e-4);
      bbox.extend(bbox.min() - Vector(0, 1.e-4, 0));
    }
    if (fabs(size.z())<1.e-4)
    {
      size.z(2.e-4);
      bbox.extend(bbox.min() - Vector(0, 0, 1.e-4));
    }
    Point center(bbox.min() + size / 2.);
    Vector sizex(size.x(), 0, 0);
    Vector sizey(0, size.y(), 0);
    Vector sizez(0, 0, size.z());

    Point right(center + sizex / 2.);
    Point down(center + sizey / 2.);
    Point in(center + sizez / 2.);

    Transform r;
    Point unused;
    field_initial_transform_.load_identity();

    double sx = (right - center).length();
    double sy = (down - center).length();
    double sz = (in - center).length();

    if (sx < 1e-12) sx = 1.0;
    if (sy < 1e-12) sy = 1.0;
    if (sz < 1e-12) sz = 1.0;

    field_initial_transform_.pre_scale(Vector(sx, sy, sz));
    r.load_frame((right - center).safe_normal(),
      (down - center).safe_normal(),
      (in - center).safe_normal());

    field_initial_transform_.pre_trans(r);
    field_initial_transform_.pre_translate(center.asVector());

    resetting_.set(0);
  }

  if (useoutputsize_.get() || useoutputcenter_.get())
  {
    Point center, right, down, in;
    outputcenterx_.reset(); outputcentery_.reset(); outputcenterz_.reset();
    outputsizex_.reset(); outputsizey_.reset(); outputsizez_.reset();
    if (outputsizex_.get() < 0 ||
      outputsizey_.get() < 0 ||
      outputsizez_.get() < 0)
    {
      //begin bug fix: avoid degenerated bbox, M. Dannhauer, 05/29/14
      if (outputsizex_.get() < 0) outputsizex_.set(-outputsizex_.get());
      if (outputsizey_.get() < 0) outputsizey_.set(-outputsizey_.get());
      if (outputsizez_.get() < 0) outputsizez_.set(-outputsizez_.get());
      // error("Degenerate BBox requested.");
      // return;                    // degenerate
      //end bug fix: avoid degenerated bbox, M. Dannhauer, 05/29/14
    }
    Vector sizex, sizey, sizez;
    box_->GetPosition(center, right, down, in);
    if (useoutputsize_.get())
    {
      sizex = Vector(outputsizex_.get(), 0, 0);
      sizey = Vector(0, outputsizey_.get(), 0);
      sizez = Vector(0, 0, outputsizez_.get());
    }
    else
    {
      sizex = (right - center) * 2;
      sizey = (down - center) * 2;
      sizez = (in - center) * 2;
    }
    if (useoutputcenter_.get())
    {
      center = Point(outputcenterx_.get(),
        outputcentery_.get(),
        outputcenterz_.get());
    }
    right = Point(center + sizex / 2.);
    down = Point(center + sizey / 2.);
    in = Point(center + sizez / 2.);

    box_->SetPosition(center, right, down, in);
  }

  // Transform the mesh if necessary.
  // Translate * Rotate * Scale.

  Point center, right, down, in;
  box_->GetPosition(center, right, down, in);

  Transform t, r;
  Point unused;
  t.load_identity();
  t.pre_scale(Vector((right - center).length(),
    (down - center).length(),
    (in - center).length()));
  r.load_frame((right - center).safe_normal(),
    (down - center).safe_normal(),
    (in - center).safe_normal());
  t.pre_trans(r);
  t.pre_translate(center.asVector());


  Transform inv(field_initial_transform_);
  inv.invert();
  t.post_trans(inv);

  // Change the input field handle here.
  fh.detach();

  fh->mesh_detach();
  fh->vmesh()->transform(t);

  send_output_handle("Output Field", fh);

  // Convert the transform into a matrix and send it out.
  MatrixHandle mh = new DenseMatrix(t);
  send_output_handle("Transformation Matrix", mh);
#endif
}

void
EditMeshBoundingBox::widget_moved(bool last)
{
#ifdef WORKING_ON_EDITMESH_DAN
  if (last)
  {
    Point center, right, down, in;
    outputcenterx_.reset(); outputcentery_.reset(); outputcenterz_.reset();
    outputsizex_.reset(); outputsizey_.reset(); outputsizez_.reset();
    box_->GetPosition(center, right, down, in);
    outputcenterx_.set(center.x());
    outputcentery_.set(center.y());
    outputcenterz_.set(center.z());
    outputsizex_.set((right.x() - center.x())*2.);
    outputsizey_.set((down.y() - center.y())*2.);
    outputsizez_.set((in.z() - center.z())*2.);
    box_mode_.set(box_->GetMode());
    box_center_.set(center);
    box_right_.set(right);
    box_down_.set(down);
    box_in_.set(in);
    box_scale_.set(1.0);
    want_to_execute();
  }
  box_real_scale_.set(box_->GetScale());
#endif
}

BoxWidgetPtr WidgetFactory::createBox()
{
  return boost::make_shared<BoxWidgetNull>();
}

const AlgorithmParameterName EditMeshBoundingBox::InputCenterX("InputCenterX");
const AlgorithmParameterName EditMeshBoundingBox::InputCenterY("InputCenterY");
const AlgorithmParameterName EditMeshBoundingBox::InputCenterZ("InputCenterZ");
const AlgorithmParameterName EditMeshBoundingBox::InputSizeX("InputSizeX");
const AlgorithmParameterName EditMeshBoundingBox::InputSizeY("InputSizeY");
const AlgorithmParameterName EditMeshBoundingBox::InputSizeZ("InputSizeZ");
 //Output Field Atributes
const AlgorithmParameterName EditMeshBoundingBox::UseOutputCenter("UseOutputCenter");
const AlgorithmParameterName EditMeshBoundingBox::UseOutputSize("UseOutputSize");
const AlgorithmParameterName EditMeshBoundingBox::OutputCenterX("OutputCenterX");
const AlgorithmParameterName EditMeshBoundingBox::OutputCenterY("OutputCenterY");
const AlgorithmParameterName EditMeshBoundingBox::OutputCenterZ("OutputCenterZ");
const AlgorithmParameterName EditMeshBoundingBox::OutputSizeX("OutputSizeX");
const AlgorithmParameterName EditMeshBoundingBox::OutputSizeY("OutputSizeY");
const AlgorithmParameterName EditMeshBoundingBox::OutputSizeZ("OutputSizeZ");
//Widget Scale/Mode
const AlgorithmParameterName EditMeshBoundingBox::DoubleScaleUp("DoubleScaleUp");
const AlgorithmParameterName EditMeshBoundingBox::ScaleUp("ScaleUp");
const AlgorithmParameterName EditMeshBoundingBox::ScaleDown("ScaleDown");
const AlgorithmParameterName EditMeshBoundingBox::DoubleScaleDown("DoubleScaleDown");
const AlgorithmParameterName EditMeshBoundingBox::NoTranslation("NoTranslation");
const AlgorithmParameterName EditMeshBoundingBox::XYZTranslation("XYZTranslation");
const AlgorithmParameterName EditMeshBoundingBox::RDITranslation("RDITranslation");
const AlgorithmParameterName EditMeshBoundingBox::RestrictX("RestrictX");
const AlgorithmParameterName EditMeshBoundingBox::RestrictY("RestrictY");
const AlgorithmParameterName EditMeshBoundingBox::RestrictZ("RestrictZ");
const AlgorithmParameterName EditMeshBoundingBox::RestrictR("RestrictR");
const AlgorithmParameterName EditMeshBoundingBox::RestrictD("RestrictD");
const AlgorithmParameterName EditMeshBoundingBox::RestrictI("RestrictI");
