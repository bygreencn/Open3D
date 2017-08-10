// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2017 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "py3d_visualization.h"

#include <Core/Core.h>
#include <Visualization/Visualization.h>
#include <IO/ClassIO/IJsonConvertibleIO.h>
using namespace three;

void pybind_utility(py::module &m)
{
	py::class_<SelectionPolygonVolume> selection_volume(m,
			"SelectionPolygonVolume");
	py::detail::bind_default_constructor<SelectionPolygonVolume>(
			selection_volume);
	py::detail::bind_copy_functions<SelectionPolygonVolume>(selection_volume);
	selection_volume
		.def("CropPointCloud", [](const SelectionPolygonVolume &s,
				const PointCloud &input) {
			return s.CropPointCloud(input);
		}, "input"_a)
		.def("__repr__", [](const SelectionPolygonVolume &s) {
			return std::string("SelectionPolygonVolume, access its members:\n"
					"orthogonal_axis, bounding_polygon, axis_min, axis_max");
		})
		.def_readwrite("orthogonal_axis",
				&SelectionPolygonVolume::orthogonal_axis_)
		.def_readwrite("bounding_polygon",
				&SelectionPolygonVolume::bounding_polygon_)
		.def_readwrite("axis_min", &SelectionPolygonVolume::axis_min_)
		.def_readwrite("axis_max", &SelectionPolygonVolume::axis_max_);
}

void pybind_utility_methods(py::module &m)
{
	m.def("DrawGeometries", 
	[](const std::vector<std::shared_ptr<const Geometry>> &geometry_ptrs,
			const std::string &window_name, int width, int height,
			int left, int top) {
		std::string current_dir = filesystem::GetWorkingDirectory();
		DrawGeometries(geometry_ptrs, window_name, width, height, left, top);
		filesystem::ChangeWorkingDirectory(current_dir);
	}, "Function to draw a list of Geometry objects",
			"geometry_list"_a, "window_name"_a = "Open3D", "width"_a = 1920,
			"height"_a = 1080, "left"_a = 50, "top"_a = 50);
	m.def("DrawGeometriesWithCustomAnimation",
	[](const std::vector<std::shared_ptr<const Geometry>> &geometry_ptrs,
			const std::string &window_name, int width, int height,
			int left, int top, const std::string &json_filename) {
		std::string current_dir = filesystem::GetWorkingDirectory();
		DrawGeometriesWithCustomAnimation(geometry_ptrs, window_name, width,
				height, left, top, json_filename);
		filesystem::ChangeWorkingDirectory(current_dir);
	}, "Function to draw a list of Geometry objects",
			"geometry_list"_a, "window_name"_a = "Open3D", "width"_a = 1920,
			"height"_a = 1080, "left"_a = 50, "top"_a = 50,
			"optional_view_trajectory_json_file"_a = "");
	m.def("ReadSelectionPolygonVolume", [](const std::string &filename) {
		SelectionPolygonVolume vol;
		ReadIJsonConvertible(filename, vol);
		return vol;
	}, "Function to read SelectionPolygonVolume from file", "filename"_a);
}
