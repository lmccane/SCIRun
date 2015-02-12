/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2010 Scientific Computing and Imaging Institute,
   University of Utah.

   
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

#ifndef MODULES_LEGACY_MATH_COMPUTESVD_H_
#define MODULES_LEGACY_MATH_COMPUTESVD_H_ 1

#include <Dataflow/Network/Module.h>
#include <Modules/Legacy/Math/share.h>

namespace SCIRun {
	namespace Modules {
		namespace Math {
		
		class SCISHARE ComputeSVD : public Dataflow::Networks::Module,
			public Has1InputPort<MatrixPortTag>,
			public Has3OutputPorts<MatrixPortTag, MatrixPortTag, MatrixPortTag>
			{
				public:
					ComputeSVD();
					virtual void setStateDefaults() {}
					virtual void execute();
					
					INPUT_PORT(0, InputMatrix, Matrix);
					OUTPUT_PORT(0, LeftSingularMatrix, DenseMatrix);
					OUTPUT_PORT(1, SingularValues, DenseMatrix);
					OUTPUT_PORT(2, RightSingularMatrix, DenseMatrix);
			
			};
			
}}};

#endif