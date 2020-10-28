%module HKIPcamera
%include <opencv/mat.i>
%cv_mat__instantiate_defaults
%header %{
    #include "HKIPcamera.h"
%}

%include "HKIPcamera.h"
