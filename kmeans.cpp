#include <opencv2/nonfree/nonfree.hpp>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/ocl/ocl.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/legacy/compat.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace cv;
using namespace std;
const int DIM = 128;
const int SURF_PARAM = 400;
const int MAX_CLUSTER = 100;//kの数

int surf(Mat img,Mat &imgs){
	//SURF
	SurfFeatureDetector detector;
	SurfDescriptorExtractor extractor;
	extractor.extended = true;//特徴ベクトルを１２８次元にする
//	SURF img_surf = SURF(500, 4, 2, true);//SURFを初期化
	vector<KeyPoint> kp;
//	img_surf(img, Mat(), kp);// vector<KeyPoint> keypoints;
	detector.detect(img, kp);
	extractor.compute(img, kp, imgs);
	/*
	vector<KeyPoint>::iterator it = kp.begin(), it_end = kp.end();//ベクター内をすべて走査
	for (; it != it_end; ++it) {
		circle(img, Point(it->pt.x, it->pt.y),
			saturate_cast<int>(it->size*0.25), Scalar(255, 255, 0));//○を描く(○を書く画像,中心座標,半径,色)
	}*/
//	imgs = img;
	return 0;
}

int loadDescriptors(Mat imgld,Mat& featureVectors) {


	vector< vector<float> > samples;


		// SURFを抽出
		Mat descriptors;
		int ret = surf(imgld, descriptors);
		if (ret != 0) {
			cerr << "error in extractSURF" << endl;
			return 1;
		}
		for (int i = 0; i < descriptors.rows; i++){
			samples.push_back(descriptors.row(i));
		}
		// ファイル名と局所特徴点の数を表示
	//	cout << filePath << "\t" << descriptors.rows << endl;
	
	featureVectors.resize(samples.size());
	for (int i = 0; i < samples.size(); i++){
		for (int j = 0; j < samples[i].size(); j++){
			featureVectors.at<float>(i, j) = samples[i][j];
		}
	}
	return 0;
}

int calcHistograms(Mat imgch, Mat& visualWords) {
	flann::Index idx(visualWords, flann::KDTreeIndexParams(4), cvflann::FLANN_DIST_L2);

	// 各画像のヒストグラムを出力するファイルを開く

	ofstream file("histograms.txt", ios::app);//ヒストグラム書き込み用
	if (file.fail()){
		cerr << "failed." << endl;
		exit(0);
	}
	//ファイル名を入力
	std::string str;
	std::cin >> str;
	std::cout << str << std::endl;

	//写真を保存
	imwrite(str + ".jpg", imgch);
	
		// ヒストグラムを初期化
		int* histogram = new int[visualWords.rows];

		for (int i = 0; i < visualWords.rows; i++) {
			histogram[i] = 0;
		}

		Mat descriptors;
		int ret = surf(imgch, descriptors);
		if (ret != 0) {
			cerr << "error in extractSURF" << endl;
			return 1;
		}
		int maxResults = 1;
		float r = 1.0f;
		for (int i = 0; i < descriptors.rows; i++){
			Mat indices = (Mat_<int>(1, 1)); // dataの何行目か
			Mat dists = (Mat_<float>(1, 1)); // それぞれどれだけの距離だったか
			Mat query = (Mat_<float>(1, descriptors.cols));
			//descriptors.col(i).isContinuous()がfalseなので新しく行列queryを作る
			for (int m = 0; m < descriptors.cols; m++){
				query.at<float>(0, m) = descriptors.at<float>(i, m);
			}
			idx.radiusSearch(query, indices, dists, r, maxResults);
			histogram[indices.at<int>(0, 0)] += 1;
		}

		// ヒストグラムをファイルに出力
		if (0 != descriptors.rows){
			file << str << "\t";
			for (int i = 0; i < visualWords.rows; i++) {
				file << float(histogram[i]) / float(descriptors.rows) << "\t";
			}
			file << endl;
		}
		// 後始末
		delete[] histogram;


	file.close();

	return 0;
}

int main(int argc, char *argv[]){
	initModule_nonfree();

	ofstream file("histograms.txt", ios::app);//ヒストグラム書き込み用
	if (file.fail()){
		cerr << "failed." << endl;
		exit(0);
	}

	// 変数宣言
	Mat im;
	Mat imc;
//	int flag = 0;
	// カメラのキャプチャ
	VideoCapture cap(0);
	// キャプチャのエラー処理
	if (!cap.isOpened()) return -1;

	while (1) {
		// カメラ映像の取得
		cap >> im;
		// 映像の表示
		imshow("Camera", im);
//		if (waitKey(30) == 'e') break;//eを押すと終わり
		// sキー入力があればキャプチャ
		if (waitKey(30) == 's'){
			cap >> imc;
			break;
//			flag = 1;
		}

	}
	// IMAGE_DIRの各画像から局所特徴量を抽出
	cout << "Load Descriptors ..." << endl;
	Mat featureVectors = (Mat_<float>(1, DIM));
	//特徴ベクトルを取り出し、学習用DBを作る
	int ld = loadDescriptors(imc, featureVectors);
	// 局所特徴量をクラスタリングして各クラスタのセントロイドを計算
	cout << "Clustering ..." << endl;
	Mat labels(featureVectors.rows, 1, CV_64F);        // 各サンプル点が割り当てられたクラスタのラベル
	//Mat centroids(MAX_CLUSTER, DIM, CV_64F);  // 各クラスタの中心（セントロイド） DIM次元ベクトル
	Mat centroids = Mat_<float>(MAX_CLUSTER, DIM);
	kmeans(featureVectors, MAX_CLUSTER, labels, cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 1, KMEANS_PP_CENTERS, centroids);

	// 各画像をVisual Wordsのヒストグラムに変換する
	// 各クラスターの中心ベクトル、centroidsがそれぞれVisual Wordsになる
	cout << "Calc Histograms ..." << endl;
	calcHistograms(imc,centroids);
	/*
	// IMAGE_DIRの各画像から局所特徴量を抽出
	cout << "Load Descriptors ..." << endl;
	Mat featureVectors = (Mat_<float>(1, DIM));
	cout << featureVectors.rows << "\n";
	//特徴ベクトルを取り出し、学習用DBを作る
	int ret = loadDescriptors(featureVectors);

	// 局所特徴量をクラスタリングして各クラスタのセントロイドを計算
	cout << "Clustering ..." << endl;
	Mat labels(featureVectors.rows, 1, CV_64F);        // 各サンプル点が割り当てられたクラスタのラベル
	//Mat centroids(MAX_CLUSTER, DIM, CV_64F);  // 各クラスタの中心（セントロイド） DIM次元ベクトル
	Mat centroids = Mat_<float>(MAX_CLUSTER, DIM);
	kmeans(featureVectors, MAX_CLUSTER, labels, cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0), 1, KMEANS_PP_CENTERS, centroids);

	// 各画像をVisual Wordsのヒストグラムに変換する
	// 各クラスターの中心ベクトル、centroidsがそれぞれVisual Wordsになる
	cout << "Calc Histograms ..." << endl;
	calcHistograms(centroids);
	*/

	return 0;

}



