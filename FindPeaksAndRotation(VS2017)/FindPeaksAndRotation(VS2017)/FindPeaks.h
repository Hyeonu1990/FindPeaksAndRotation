#pragma once

#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <list>

using namespace cv;
using namespace std;


class FindPeaks
{
public:

	Mat result;
	float angle = 0;
	float scale = 1;
	bool debug = true;
	/*
	@param src : 처리할 이미지
	@param startBlockSize : peak 탐색 블럭 시작 크기
	@param MaxBlockSize : peak 탐색 블럭 최대 크기
	@param Increase : 루프문을 돌때마다 증가하는 블럭 크기
	*/
	FindPeaks(Mat* src, int startBlockSize = 40, int MaxBlockSize = 0, int Increase = 4)  //src - 웨이블릿 변환 이미지
	{
		if (src->channels() > 1) cvtColor(*src, temp, CV_BGR2GRAY);
		else temp = src->clone();

		if (temp.type() != CV_32FC1) temp.convertTo(temp, CV_32FC1);

		int startSize = startBlockSize; //peak 탐색 블럭 시작크기

		int MaxSize;
		if (MaxBlockSize == 0) MaxSize = temp.cols / 5;
		else MaxSize = MaxBlockSize;

		int increase = Increase;

		FindAllPeaks(startSize, MaxSize, increase);

		Find3peaksCombinations();

		FindWantedLines();		

		if (debug)
		{
			Mat output;
			//Mat compare(temp.rows, temp.cols, CV_32FC1);
			temp.convertTo(output, CV_8UC1);
			imshow("temp", output);
			imwrite("./image/peaks_lines.png", output);
		}

		angle = -acosf(AnglesMaxValue()) * 180 / CV_PI;
		if (debug) printf("first angle value : %f\n", angle);

		if (angle == 90.0f || angle == -90.0f) angle = 0;
		else if (angle > 135.0f) angle -= 180;
		else if (angle <= 135.0f && angle > 90) angle -= 90;
		else if (angle >= -135.0f && angle < -90) angle += 90;
		else if (angle < -135.0f) angle += 180;
		//else 
		if(lineLength != 0)
			scale = 256 / lineLength; //1;
		if(debug) printf("MatAngleValue : %f\nacosf(AnglesMaxValue()) = %f\nangle = %f\nscale : %f\n", AnglesMaxValue(), acosf(AnglesMaxValue()), angle, scale);


		result = temp;

	}
private:

	Mat temp; // input 이미지를 복사할 작업용 변수

	list<Point> peaks; // 모든 peak 점들
	list<Point*> _3peaks; // peak 점들 중복없이 3개씩
	list<Point*> lines; // _3peaks 중에서 각도조건에 맞는 직선들
	list<Point*> square_lines; // lines 중에서 정사각형 조건에 맞는 직선들

	/*각도값 및 인식횟수 보관 리스트
	float{ 각도, 길이, 인식횟수}
	*/
	list<float*> angles;

	list<Point>::iterator peaks_ptr; // peaks 리스트 탐색을 위한 변수

	/*모든 peak를 찾는 함수
	@param startSize : peak를 찾기 시작할 범위(정사각형 블럭)의 변 길이
	@param increase : 범위박스 크기 증가율 (기본값 4)
	*/
	void FindAllPeaks(int startSize, int maxSize, int increase = 4)
	{
		int size = startSize; //원본이미지 크기에 따라 조절해야할듯

		for (; size < maxSize; size += increase)
		{
			peaks.clear();

			for (int x = 0; x < temp.rows; x += size)
				for (int y = 0; y < temp.cols; y += size)
				{
					//if ( ((x + size) >= temp.rows) || ((y + size) >= temp.cols) ) continue;
					FindPeak(&temp, Point(x, y), size);
				}
			for (int x = temp.rows - 1; x >= 0; x -= size)
				for (int y = temp.cols - 1; y >= 0; y -= size)
				{
					//if (((x - size) < 0) || ((y - size) < 0)) continue;
					FindPeak(&temp, Point(x, y), size, true);
				}

			Mat peaksSum = Mat::zeros(temp.rows, temp.cols, CV_32FC1);
			Mat saveimg; if (debug) saveimg = peaksSum.clone();
			peaks_ptr = peaks.begin();
			for (int i = 0; i < peaks.size(); i++)
			{
				peaksSum.at<float>(peaks_ptr->y, peaks_ptr->x) = temp.at<float>(peaks_ptr->y, peaks_ptr->x);

				if (debug)
					for (int draw_x = peaks_ptr->x - 1; draw_x <= peaks_ptr->x + 1; draw_x++)
						for (int draw_y = peaks_ptr->y - 1; draw_y <= peaks_ptr->y + 1; draw_y++)
							if (draw_x >= 0 && draw_x < temp.rows && draw_y >= 0 && draw_y < temp.cols)
								saveimg.at<float>(draw_y, draw_x) = 255;

				peaks_ptr++;
			}

			temp = peaksSum;

			if (debug)
			{
				saveimg.convertTo(saveimg, CV_8UC1);
				imshow("temp size" + to_string(size), saveimg);
				//imwrite("./image/FindPeaks_size" + to_string(size) + ".png", saveimg);
			}

		}
	}

	/*peaks 리스트에 push 전 중복체크함수
	FindAllPeaks에서 역탐색 중 발견된 peak가 기존에 등록된 peak인지 확인
	@param value : 역탐색 중 발견된 peak 좌표값
	*/
	bool ListOverlapCheck(Point value)
	{
		peaks_ptr = peaks.begin();

		for (int i = 0; i < peaks.size(); i++)
		{
			if ((peaks_ptr->x == value.x) && (peaks_ptr->y == value.y))
				return false;
			peaks_ptr++;
		}

		return true;
	}

	/*지정한 범위 내 peak값 검출
	src 이미지에서 startPoint부터 size크기의 블럭범위 내의 피크값 추출
	@param src : peak값을 검출할 이미지
	@param startPoint : peak 검출 시작 지점
	@param size : 정사각형 블럭의 변 길이
	@param reverse : 역변환 여부
	*/
	void FindPeak(Mat* src, Point startPoint, int size, bool reverse = false)
	{
		Point MaxPoint;
		float MaxValue;

		if (!reverse)
		{
			for (int x = startPoint.x; x < (startPoint.x + size); x++)
			{
				for (int y = startPoint.y; y < (startPoint.y + size); y++)
				{
					if ((y >= src->cols) || (x >= src->rows))
						continue;
					if (x == startPoint.x && y == startPoint.y)
					{
						MaxPoint = Point(x, y);
						MaxValue = src->at<float>(y, x);
					}
					else
					{
						if (src->at<float>(y, x) > MaxValue)
						{
							MaxValue = src->at<float>(y, x);
							MaxPoint = Point(x, y);
						}
					}
				}
			}
		}
		else
		{
			for (int x = startPoint.x; x > (startPoint.x - size); x--)
			{
				for (int y = startPoint.y; y > (startPoint.y - size); y--)
				{
					if ((y < 0) || (x < 0))
						continue;
					if (x == startPoint.x && y == startPoint.y)
					{
						MaxPoint = Point(x, y);
						MaxValue = src->at<float>(y, x);
					}
					else
					{
						if (src->at<float>(y, x) > MaxValue)
						{
							MaxValue = src->at<float>(y, x);
							MaxPoint = Point(x, y);
						}
					}
				}
			}
		}

		if (MaxValue != 0)
		{
			if (!reverse || ListOverlapCheck(MaxPoint))
				peaks.push_back(MaxPoint);
		}

	}


	//---주로 Find3peaksLine, next_combi 함수에서 사용하는 변수들

	int elementNum[3] = { 0,1,2 };
	int* peaksNum; // peaks 갯수에 맞게 생성되는 0부터 시작하는 숫자 배열
	int PredictedCount = 0; // 예상되는 3peaks 조합 갯수
	int loopCount = 0; // while 루프문을 돈 횟수

	//중복없는 3개의 조합을 찾기 위한 combination 함수
	int next_combi() {
		int i = 2;
		while (i >= 0 && elementNum[i] == peaks.size() - 3 + i) i--;
		if (i<0) return 0;
		elementNum[i]++;
		for (int j = i + 1; j<3; j++) elementNum[j] = elementNum[i] + j - i;
		return 1;

	}

	//모든 peaks 중에서 중복없는 3개의 조합을 찾고 그 중에서도 특별한 조건을 가진 조합들을 3peaks 리스트에 저장
	int Find3peaksCombinations() {
		if (peaks.size() == 0) return 0;

		peaksNum = new int[peaks.size()];
		for (int i = 0; i < peaks.size(); i++)	peaksNum[i] = i;

		PredictedCount = peaks.size() * (peaks.size() - 1) * (peaks.size() - 2) / (3 * 2 * 1);

		//_3peaks = new Point*[count];

		list<Point*>::iterator _3peaks_ptr;
		_3peaks.push_back(new Point[3]);
		_3peaks_ptr = _3peaks.begin();

		loopCount = 0;

		do {

			for (int j = 0; j < 3; j++)
			{
				auto ptr = peaks.begin();
				int num = peaksNum[elementNum[j]];
				std::advance(ptr, num);
				(*_3peaks_ptr)[j] = *ptr;
			}
			int lineCheck = FindPeaksLine((*_3peaks_ptr)[0], (*_3peaks_ptr)[1], (*_3peaks_ptr)[2]);
			if (lineCheck)
			{
				Point temp;
				switch (lineCheck)
				{
				case 1: // p1, p2, p3
					break;

				case 2: // p2, p1, p3					
					temp = (*_3peaks_ptr)[0];
					(*_3peaks_ptr)[0] = (*_3peaks_ptr)[1];
					(*_3peaks_ptr)[1] = temp;
					break;

				case 3: // p1, p3, p2
					temp = (*_3peaks_ptr)[1];
					(*_3peaks_ptr)[1] = (*_3peaks_ptr)[2];
					(*_3peaks_ptr)[2] = temp;
					break;

				default:
					break;
				}
				if (loopCount == PredictedCount - 1) break;
				_3peaks.push_back(new Point[3]);
				_3peaks_ptr++;
			}

			if (loopCount == PredictedCount - 1)
			{
				_3peaks.pop_back();
				break;
			}
			loopCount++;
		} while (next_combi());

		return 0;

	}

	/*3개의 peak 점들이 직선을 이루는지 확인하는 함수
	직선이 되는 조합에 따라 int값을 return
	@param p1, p2, p3 : 3peaks에 들어갈 좌표들

	@return 1 : p1, p2, p3 순서로 직선일 경우
	@return 2 : p2, p1, p3 순서로 직선일 경우
	@return 3 : p1, p3, p2 순서로 직선일 경우
	@return 0 : 어떠한 조건에서도 직선이 성립되지 않을 경우
	*/
	int FindPeaksLine(Point p1, Point p2, Point p3)
	{
		float dif_ab_cut = 10.0f;
		float dist_c_cut = 5.0f;

		if (Dif_dist_AB(p1, p2, p3) <= dif_ab_cut && FindDistC(p1, p2, p3) <= dist_c_cut)
		{
			if (debug)
			{
				printf("n: %d | Dif_dist_AB(p1, p2, p3) : %f, FindDistC(p1, p2, p3) : %f\n", loopCount, Dif_dist_AB(p1, p2, p3), FindDistC(p1, p2, p3));
				cout << loopCount << " : " << p1 << ", " << p2 << ", " << p3 << endl;
			}
			return 1;
		}

		if (Dif_dist_AB(p2, p1, p3) <= dif_ab_cut && FindDistC(p2, p1, p3) <= dist_c_cut)
		{
			if (debug)
			{
				printf("n: %d | Dif_dist_AB(p2, p1, p3) : %f, FindDistC(p2, p1, p3) : %f\n", loopCount, Dif_dist_AB(p2, p1, p3), FindDistC(p2, p1, p3));
				cout << loopCount << " : " << p1 << ", " << p2 << ", " << p3 << endl;
			}
			return 2;
		}

		if (Dif_dist_AB(p1, p3, p2) <= dif_ab_cut && FindDistC(p1, p3, p2) <= dist_c_cut)
		{
			if (debug)
			{
				printf("n: %d | Dif_dist_AB(p1, p3, p2) : %f, FindDistC(p1, p3, p2) : %f\n", loopCount, Dif_dist_AB(p1, p3, p2), FindDistC(p1, p3, p2));
				cout << loopCount << " : " << p1 << ", " << p2 << ", " << p3 << endl;
			}
			return 3;
		}

		return 0;
	}

	/*중간점을 기준으로 각 점들의 길이 차의 절대값을 구하는 함수
	p1-p2 직선(A), p2-p3 직선(B) 길이의 차의 절대값을 return
	@param p1, p2, p3 : FindPeaksLine에 주어진 좌표들
	*/
	float Dif_dist_AB(Point p1, Point p2, Point p3)
	{
		float distA = sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2));
		float distB = sqrt(pow((p2.x - p3.x), 2) + pow((p2.y - p3.y), 2));

		return abs(distA - distB);
	}

	/*중심으로 선택된 점과 나머지 두 점의 중간점의 길이를 구하는 함수
	중심점 p2와 나머지 p1, p3의 중심점 사이의 거리를 구하는 함수
	@param p1, p2, p3 : FindPeaksLine에 주어진 좌표들
	*/
	float FindDistC(Point p1, Point p2, Point p3)
	{
		Point p4;

		if ((p3.x - p1.x) != 0 && (p3.y - p1.y) != 0) //p1-p3 직선이 수평 또는 수직이 아닐때
		{
			//y = m1x + b1 : p1, p3, p4
			double m1 = (double)(p3.y - p1.y) / (double)(p3.x - p1.x); // p1 - p3 기울기
			double b1 = (double)p1.y - (double)(m1 * p1.x);

			//y = m2x + b2 : p2, p4
			double m2 = -1 / m1;
			double b2 = (double)p2.y - (double)(p2.x * m2);


			p4.x = (b2 - b1) / (m1 - m2);//(p4.y - b1) / m1;
			p4.y = (b1 * m2 - b2 * m1) / (m2 - m1);
		}
		else if ((p3.x - p1.x) == 0) //p1-p3 직선이 수직일때
		{
			p4.x = p1.x;
			p4.y = p2.y;
		}
		else if ((p3.y - p1.y) == 0) //p1-p3 직선이 수평일때
		{
			p4.x = p2.x;
			p4.y = p1.y;
		}

		float distC = sqrt(pow((p2.x - p4.x), 2) + pow((p2.y - p4.y), 2));

		return distC;
	}

	//3peaks 값들 중에서 일정 각도를 만족하는 조건의 직선을 찾고 그려주는 함수
	void FindWantedLines()
	{
		list<Point*>::iterator _3peaks_ptr;
		_3peaks_ptr = _3peaks.begin();
		if (debug) printf("_3peaks.size() : %d\n", _3peaks.size());
		for (int n = 0; n < _3peaks.size(); n++)
		{
			if (AngleCheck((*_3peaks_ptr)[0], (*_3peaks_ptr)[2]))
			{
				lines.push_back((*_3peaks_ptr));

				if (debug)
				{
					line(temp, (*_3peaks_ptr)[0], (*_3peaks_ptr)[2], Scalar(255));
					for (int i = -2; i <= 2; i++)
					{
						for (int j = -2; j <= 2; j++)
						{
							for (int nm = 0; nm < 3; nm++)
							{
								int y = (*_3peaks_ptr)[nm].y + i;
								int x = (*_3peaks_ptr)[nm].x + j;
								if (y >= 0 && y < temp.cols	&& x >= 0 && x < temp.rows)
									temp.at<float>(y, x) = 255;
							}
						}
					}
				}
			}
			_3peaks_ptr++;
		}
	}

	/*p3 <- p1 벡터와 (1,0)단위벡터를 내적하여 각도와 길이를 확인하는 함수
	각도 및 길이 확인 후 AnglesOverlapCheck 함수에서 float배열 리스트에 정보 저장
	@param p1 : 벡터 시작지점 좌표
	@param p3 : 벡터 도작치점 좌표
	*/
	bool AngleCheck(Point2f p1, Point2f p3)
	{
		//	0    20    25     30    45    60    65     70    90   120    135    150    180    210     225    240   270    300    315    330
		//  1   0.94  0.906  0.86   0.7   0.5  0.422   0.34   0   -0.5   -0.7   -0.86   -1   -0.86   -0.7   -0.5    0     0.5    0.7    0.86

		Point2f pnorm = VectorNomalize(p3 - p1);
		//VectorNomalize(p3);
		Point2f std = Point2f(1, 0);

		float inner = -pnorm.x * std.x + -pnorm.y * std.y;

		//return inner;

		//if (abs(inner) > 0.92 || abs(inner) < 0.38)
		{
			if (debug)
			{
				cout << p1 << ", " << p3;
				printf("VectorLength : %f, inner : %f\n", VectorLength((p3 - p1)), inner);
			}
			AnglesOverlapCheck(inner, VectorLength((p3-p1)) );
			return true;
		}
		// else
		return false;
	}

	/*입력받은 방향벡터를 단위벡터로 변환해주는 함수
	@param pt : 단위벡터로 변환할 방향벡터
	*/
	Point2f VectorNomalize(Point2f pt)
	{
		Point2f result;

		result.x = pt.x / sqrt(pow(pt.x, 2) + pow(pt.y, 2));
		result.y = pt.y / sqrt(pow(pt.x, 2) + pow(pt.y, 2));

		return result;
	}

	/*입력받은 좌표와 (0,0) 사이 길이를 구하는 함수
	@param pt : 길이를 구할 좌표
	*/
	float VectorLength(Point2f pt)
	{
		float dist = sqrt(pow(pt.x, 2) + pow(pt.y, 2));

		return dist;
	}

	/*내적값 및 길이값을 angles 리스트에 저장하고 중복체크하는 함수
	내적값 및 길이값을 anlges 리스트에 저장하거나 중복된 값이 있으면 그 값의 인식 카운트 수를 올려준다
	@param inner : 내적값
	@param length : 길이값
	*/
	void AnglesOverlapCheck(float inner, float length)
	{
		float value = inner;
		float leng = length;

		list<float*>::iterator angles_ptr = angles.begin();

		if (angles.size() == 0)
		{
			if (length > 256) return;
			angles.push_back(new float[3]{ value, leng, 0 });
		}
		else
		{
			for (int i = 0; i < angles.size(); i++)
			{
				if ( 
					((*angles_ptr)[0] <= value + 0.01f && (*angles_ptr)[0] >= value - 0.01f) &&
					((*angles_ptr)[1] <= leng + 2.0f && (*angles_ptr)[1] >= leng - 2.0f) &&
					((*angles_ptr)[1] <= 256.0f)
					)
				{
					++(*angles_ptr)[2];
					return;
				}
				angles_ptr++;
			}

			angles.push_back(new float[3]{ value, leng, 0 });
		}
	}

	float lineLength = 0; // 가장 많이 검출된 직선의 길이

	//anlges 리스트에서 가장 높은 카운트를 가진 내적값과 길이값을 출력하는 함수
	float AnglesMaxValue()
	{
		list<float*>::iterator angles_ptr = angles.begin();

		float MaxCount = 0;
		float MaxLength = 0;
		float MaxValue = 0;

		for (int i = 0; i < angles.size(); i++)
		{
			printf("value : %f, Length : %f, count : %f\n", (*angles_ptr)[0], (*angles_ptr)[1], (*angles_ptr)[2]);
			if ((i == 0) || (MaxCount < (*angles_ptr)[2]))
			{
				MaxValue = (*angles_ptr)[0];
				MaxLength = (*angles_ptr)[1];
				MaxCount = (*angles_ptr)[2];
			}

			angles_ptr++;
		}
		printf("MaxValue : %f, MaxLength : %f, MaxCount : %f\n", MaxValue, MaxLength, MaxCount);
		float result = MaxValue;
		lineLength = MaxLength;

		return result;
	}	
};