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

#include <iostream>
#include <memory>

#include <flann/flann.hpp>

#include <Core/Core.h>
#include <IO/IO.h>
#include <Visualization/Visualization.h>

class KDTreeFlannFeature
{
public:
	KDTreeFlannFeature() {}
	~KDTreeFlannFeature() {}
	
public:
	bool LoadFromFile(const std::string &filename) {
		FILE* fid = fopen(filename.c_str(), "rb");
		fread(&dataset_size_, sizeof(int), 1, fid);
		fread(&dimension_, sizeof(int), 1, fid);
		data_.resize(dataset_size_ * dimension_);
		for (int i = 0; i < dataset_size_; i++) {
			Eigen::Vector3f pts;
			fread(&pts(0), sizeof(float), 3, fid);
			fread(((float *)data_.data()) + i * dimension_, sizeof(float),
					dimension_, fid);
		}
		flann_dataset_.reset(new flann::Matrix<float>((float *)data_.data(),
				dataset_size_, dimension_));
		flann_index_.reset(new flann::Index<flann::L2<float>>(*flann_dataset_,
				flann::KDTreeSingleIndexParams(15)));
		flann_index_->buildIndex();
		return true;
	}

	int SearchKNN(std::vector<float> &data, int i, int knn,
			std::vector<int> &indices, std::vector<float> &distance2) {
		flann::Matrix<float> query_flann(
				((float *)data.data()) + i * dimension_, 1, dimension_);
		indices.resize(knn);
		distance2.resize(knn);
		flann::Matrix<int> indices_flann(indices.data(), query_flann.rows, knn);
		flann::Matrix<float> dists_flann(distance2.data(), query_flann.rows,
				knn);
		return flann_index_->knnSearch(query_flann, indices_flann, dists_flann,
				knn, flann::SearchParams(-1, 0.0));
	}

public:
	std::vector<float> data_;
	std::unique_ptr<flann::Matrix<float>> flann_dataset_;
	std::unique_ptr<flann::Index<flann::L2<float>>> flann_index_;
	int dimension_ = 0;
	int dataset_size_ = 0;
};

void PrintHelp()
{
	printf("Usage:\n");
	printf("    > EvaluateFeatureMatch [options]\n");
	printf("      Evaluate feature matching quality of point clouds.\n");
	printf("\n");
	printf("Basic options:\n");
	printf("    --help, -h                : Print help information.\n");
	printf("    --log file                : A log file of the pairwise matching results. Must have.\n");
	printf("    --dir directory           : The directory storing all data files. By default it is the parent directory of the log file + pcd/.\n");
	printf("    --threshold t             : Threshold to determine if a match is good or not. Default: 0.075.\n");
	printf("    --verbose n               : Set verbose level (0-4). Default: 2.\n");
}

bool ReadLogFile(const std::string &filename,
		std::vector<std::pair<int, int>> &pair_ids,
		std::vector<Eigen::Matrix4d> &transformations)
{
	using namespace three;
	pair_ids.clear();
	transformations.clear();
	FILE * f = fopen(filename.c_str(), "r");
	if (f == NULL) {
		PrintWarning("Read LOG failed: unable to open file.\n");
		return false;
	}
	char line_buffer[DEFAULT_IO_BUFFER_SIZE];
	int i, j, k;
	Eigen::Matrix4d trans;
	int total_point_num = 0;
	int total_correspondence_num = 0;
	while (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, f)) {
		if (strlen(line_buffer) > 0 && line_buffer[0] != '#') {
			if (sscanf(line_buffer, "%d %d %d", &i, &j, &k) != 3) {
				PrintWarning("Read LOG failed: unrecognized format.\n");
				return false;
			}
			if (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, f) == 0) {
				PrintWarning("Read LOG failed: unrecognized format.\n");
				return false;
			} else {
				sscanf(line_buffer, "%lf %lf %lf %lf", &trans(0,0), &trans(0,1),
						&trans(0,2), &trans(0,3));
			}
			if (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, f) == 0) {
				PrintWarning("Read LOG failed: unrecognized format.\n");
				return false;
			} else {
				sscanf(line_buffer, "%lf %lf %lf %lf", &trans(1,0), &trans(1,1),
						&trans(1,2), &trans(1,3));
			}
			if (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, f) == 0) {
				PrintWarning("Read LOG failed: unrecognized format.\n");
				return false;
			} else {
				sscanf(line_buffer, "%lf %lf %lf %lf", &trans(2,0), &trans(2,1),
						&trans(2,2), &trans(2,3));
			}
			if (fgets(line_buffer, DEFAULT_IO_BUFFER_SIZE, f) == 0) {
				PrintWarning("Read LOG failed: unrecognized format.\n");
				return false;
			} else {
				sscanf(line_buffer, "%lf %lf %lf %lf", &trans(3,0), &trans(3,1),
						&trans(3,2), &trans(3,3));
			}
			pair_ids.push_back(std::make_pair(i, j));
			transformations.push_back(trans);
		}
	}
	fclose(f);
	return true;
}

void WriteBinaryResult(const std::string &filename, std::vector<double> &data)
{
	FILE *f = fopen(filename.c_str(), "wb");
	fwrite(data.data(), sizeof(double), data.size(), f);
	fclose(f);
}

int main(int argc, char *argv[])
{
	using namespace three;

	if (argc <= 1 || ProgramOptionExists(argc, argv, "--help") ||
			ProgramOptionExists(argc, argv, "-h")) {
		PrintHelp();
		return 0;
	}
	
	int verbose = GetProgramOptionAsInt(argc, argv, "--verbose", 2);
	SetVerbosityLevel((VerbosityLevel)verbose);
	std::string log_filename = GetProgramOptionAsString(argc, argv, "--log");
	std::string pcd_dirname = GetProgramOptionAsString(argc, argv, "--dir");
	if (pcd_dirname.empty()) {
		pcd_dirname = filesystem::GetFileParentDirectory(log_filename) +
				"pcds/";
	}
	double threshold = GetProgramOptionAsDouble(argc, argv, "--threshold",
			0.075);
	double threshold2 = threshold * threshold;
	//std::vector<std::string> features = {"fpfh", "pfh", "shot", "spin", "usc", "d32_norelu"};
	//std::vector<std::string> features = {"r17", "pcar17"};
	//std::vector<std::string> features = {"fpfh", "usc"};
	std::vector<std::string> features = {"fpfh", "d32"};

	std::vector<std::string> pcd_names;
	filesystem::ListFilesInDirectoryWithExtension(pcd_dirname, "pcd",
			pcd_names);
	std::vector<PointCloud> pcds(pcd_names.size());
	std::vector<KDTreeFlann> kdtrees(pcd_names.size());
	for (auto i = 0; i < pcd_names.size(); i++) {
		ReadPointCloud(pcd_dirname + "cloud_bin_" + std::to_string(i) + ".pcd",
				pcds[i]);
		kdtrees[i].SetGeometry(pcds[i]);
	}

	std::vector<std::pair<int, int>> pair_ids;
	std::vector<Eigen::Matrix4d> transformations;
	ReadLogFile(log_filename, pair_ids, transformations);
	int total_point_num = 0;
	int total_correspondence_num = 0;
	for (auto k = 0; k < pair_ids.size(); k++) {
		PointCloud source = pcds[pair_ids[k].second];
		source.Transform(transformations[k]);
		std::vector<int> indices(1);
		std::vector<double> distance2(1);
		int correspondence_num = 0;
		for (const auto &pt : source.points_) {
			if (kdtrees[pair_ids[k].first].SearchKNN(pt, 1, indices,
					distance2) > 0) {
				if (distance2[0] < threshold2) {
					correspondence_num++;
				}
			}
		}
		total_correspondence_num += correspondence_num;
		total_point_num += (int)source.points_.size();
		PrintInfo("#%d <-- #%d : %d out of %d (%.2f%%).\n",
				pair_ids[k].first, pair_ids[k].second, correspondence_num,
				(int)source.points_.size(),
				correspondence_num * 100.0 / source.points_.size());
	}
	PrintWarning("Total %d out of %d (%.2f%% coverage).\n\n", 
			total_correspondence_num, total_point_num,
			total_correspondence_num * 100.0 / total_point_num);

	for (const auto feature : features) {
		PrintWarning("Evaluate feature %s.\n", feature.c_str());
		std::vector<KDTreeFlannFeature> feature_trees(pcd_names.size());
#ifdef _OPENMP
#pragma omp parallel for schedule(static) num_threads(16)
#endif
		for (auto i = 0; i < pcd_names.size(); i++) {
			feature_trees[i].LoadFromFile(pcd_dirname + "cloud_bin_" + 
					std::to_string(i) + "." + feature);
		}
		PrintInfo("All KDTrees built.\n");
		int total_point_num = 0;
		int total_correspondence_num = 0;
		int total_positive = 0;

		for (auto k = 0; k < pair_ids.size(); k++) {
			PointCloud source = pcds[pair_ids[k].second];
			total_point_num += (int)source.points_.size();
		}
		std::vector<double> true_dis(total_point_num, -1.0);
		total_point_num = 0;

		for (auto k = 0; k < pair_ids.size(); k++) {
			PointCloud source = pcds[pair_ids[k].second];
			source.Transform(transformations[k]);
			std::vector<int> indices(1);
			std::vector<double> distance2(1);
			std::vector<float> fdistance2(1);
			int positive = 0;
			int correspondence_num = 0;
			std::vector<bool> has_correspondence(
					pcds[pair_ids[k].second].points_.size(), false);
			for (auto i = 0; i < source.points_.size(); i++) {
				const auto &pt = source.points_[i];
				if (kdtrees[pair_ids[k].first].SearchKNN(pt, 1, indices,
						distance2) > 0) {
					if (distance2[0] < threshold2) {
						has_correspondence[i] = true;
						correspondence_num++;
					}
				}
			}
#ifdef _OPENMP
#pragma omp parallel for schedule(static) num_threads(16) private(indices, fdistance2)
#endif
			for (auto i = 0; i < source.points_.size(); i++) {
				const auto &pt = source.points_[i];
				if (has_correspondence[i]) {
					if (feature_trees[pair_ids[k].first].SearchKNN(
							feature_trees[pair_ids[k].second].data_, i, 1,
							indices, fdistance2) > 0) {
						double new_dis = (source.points_[i] -
								pcds[pair_ids[k].first].points_[indices[0]]
								).norm();
						true_dis[total_point_num + i] = new_dis;
						if (new_dis < threshold) {
#ifdef _OPENMP
#pragma omp atomic
#endif
							positive++;
						}
					}
				}
			}
			total_correspondence_num += correspondence_num;
			total_positive += positive;
			total_point_num += (int)source.points_.size();
			PrintInfo("#%d <-- #%d : %d out of %d out of %d (%.2f%% w.r.t. correspondences).\n",
					pair_ids[k].first, pair_ids[k].second, positive,
					correspondence_num, (int)source.points_.size(),
					positive * 100.0 / correspondence_num);
		}
		PrintWarning("Total %d out of %d out of %d (%.2f%% w.r.t. correspondences).\n\n", 
				total_positive, total_correspondence_num, total_point_num,
				total_positive * 100.0 / total_correspondence_num);
		WriteBinaryResult(pcd_dirname + feature + ".bin", true_dis);
	}
	return 1;
}
