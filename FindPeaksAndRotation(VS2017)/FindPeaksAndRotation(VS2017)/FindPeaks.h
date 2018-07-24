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
	@param src : ó���� �̹���
	@param startBlockSize : peak Ž�� �� ���� ũ��
	@param MaxBlockSize : peak Ž�� �� �ִ� ũ��
	@param Increase : �������� �������� �����ϴ� �� ũ��
	*/
	FindPeaks(Mat* src, int startBlockSize = 40, int MaxBlockSize = 0, int Increase = 4)  //src - ���̺� ��ȯ �̹���
	{
		if (src->channels() > 1) cvtColor(*src, temp, CV_BGR2GRAY);
		else temp = src->clone();

		if (temp.type() != CV_32FC1) temp.convertTo(temp, CV_32FC1);

		int startSize = startBlockSize; //peak Ž�� �� ����ũ��

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

	Mat temp; // input �̹����� ������ �۾��� ����

	list<Point> peaks; // ��� peak ����
	list<Point*> _3peaks; // peak ���� �ߺ����� 3����
	list<Point*> lines; // _3peaks �߿��� �������ǿ� �´� ������
	list<Point*> square_lines; // lines �߿��� ���簢�� ���ǿ� �´� ������

	/*������ �� �ν�Ƚ�� ���� ����Ʈ
	float{ ����, ����, �ν�Ƚ��}
	*/
	list<float*> angles;

	list<Point>::iterator peaks_ptr; // peaks ����Ʈ Ž���� ���� ����

	/*��� peak�� ã�� �Լ�
	@param startSize : peak�� ã�� ������ ����(���簢�� ��)�� �� ����
	@param increase : �����ڽ� ũ�� ������ (�⺻�� 4)
	*/
	void FindAllPeaks(int startSize, int maxSize, int increase = 4)
	{
		int size = startSize; //�����̹��� ũ�⿡ ���� �����ؾ��ҵ�

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

	/*peaks ����Ʈ�� push �� �ߺ�üũ�Լ�
	FindAllPeaks���� ��Ž�� �� �߰ߵ� peak�� ������ ��ϵ� peak���� Ȯ��
	@param value : ��Ž�� �� �߰ߵ� peak ��ǥ��
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

	/*������ ���� �� peak�� ����
	src �̹������� startPoint���� sizeũ���� ������ ���� ��ũ�� ����
	@param src : peak���� ������ �̹���
	@param startPoint : peak ���� ���� ����
	@param size : ���簢�� ���� �� ����
	@param reverse : ����ȯ ����
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


	//---�ַ� Find3peaksLine, next_combi �Լ����� ����ϴ� ������

	int elementNum[3] = { 0,1,2 };
	int* peaksNum; // peaks ������ �°� �����Ǵ� 0���� �����ϴ� ���� �迭
	int PredictedCount = 0; // ����Ǵ� 3peaks ���� ����
	int loopCount = 0; // while �������� �� Ƚ��

	//�ߺ����� 3���� ������ ã�� ���� combination �Լ�
	int next_combi() {
		int i = 2;
		while (i >= 0 && elementNum[i] == peaks.size() - 3 + i) i--;
		if (i<0) return 0;
		elementNum[i]++;
		for (int j = i + 1; j<3; j++) elementNum[j] = elementNum[i] + j - i;
		return 1;

	}

	//��� peaks �߿��� �ߺ����� 3���� ������ ã�� �� �߿����� Ư���� ������ ���� ���յ��� 3peaks ����Ʈ�� ����
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

	/*3���� peak ������ ������ �̷���� Ȯ���ϴ� �Լ�
	������ �Ǵ� ���տ� ���� int���� return
	@param p1, p2, p3 : 3peaks�� �� ��ǥ��

	@return 1 : p1, p2, p3 ������ ������ ���
	@return 2 : p2, p1, p3 ������ ������ ���
	@return 3 : p1, p3, p2 ������ ������ ���
	@return 0 : ��� ���ǿ����� ������ �������� ���� ���
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

	/*�߰����� �������� �� ������ ���� ���� ���밪�� ���ϴ� �Լ�
	p1-p2 ����(A), p2-p3 ����(B) ������ ���� ���밪�� return
	@param p1, p2, p3 : FindPeaksLine�� �־��� ��ǥ��
	*/
	float Dif_dist_AB(Point p1, Point p2, Point p3)
	{
		float distA = sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2));
		float distB = sqrt(pow((p2.x - p3.x), 2) + pow((p2.y - p3.y), 2));

		return abs(distA - distB);
	}

	/*�߽����� ���õ� ���� ������ �� ���� �߰����� ���̸� ���ϴ� �Լ�
	�߽��� p2�� ������ p1, p3�� �߽��� ������ �Ÿ��� ���ϴ� �Լ�
	@param p1, p2, p3 : FindPeaksLine�� �־��� ��ǥ��
	*/
	float FindDistC(Point p1, Point p2, Point p3)
	{
		Point p4;

		if ((p3.x - p1.x) != 0 && (p3.y - p1.y) != 0) //p1-p3 ������ ���� �Ǵ� ������ �ƴҶ�
		{
			//y = m1x + b1 : p1, p3, p4
			double m1 = (double)(p3.y - p1.y) / (double)(p3.x - p1.x); // p1 - p3 ����
			double b1 = (double)p1.y - (double)(m1 * p1.x);

			//y = m2x + b2 : p2, p4
			double m2 = -1 / m1;
			double b2 = (double)p2.y - (double)(p2.x * m2);


			p4.x = (b2 - b1) / (m1 - m2);//(p4.y - b1) / m1;
			p4.y = (b1 * m2 - b2 * m1) / (m2 - m1);
		}
		else if ((p3.x - p1.x) == 0) //p1-p3 ������ �����϶�
		{
			p4.x = p1.x;
			p4.y = p2.y;
		}
		else if ((p3.y - p1.y) == 0) //p1-p3 ������ �����϶�
		{
			p4.x = p2.x;
			p4.y = p1.y;
		}

		float distC = sqrt(pow((p2.x - p4.x), 2) + pow((p2.y - p4.y), 2));

		return distC;
	}

	//3peaks ���� �߿��� ���� ������ �����ϴ� ������ ������ ã�� �׷��ִ� �Լ�
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

	/*p3 <- p1 ���Ϳ� (1,0)�������͸� �����Ͽ� ������ ���̸� Ȯ���ϴ� �Լ�
	���� �� ���� Ȯ�� �� AnglesOverlapCheck �Լ����� float�迭 ����Ʈ�� ���� ����
	@param p1 : ���� �������� ��ǥ
	@param p3 : ���� ����ġ�� ��ǥ
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

	/*�Է¹��� ���⺤�͸� �������ͷ� ��ȯ���ִ� �Լ�
	@param pt : �������ͷ� ��ȯ�� ���⺤��
	*/
	Point2f VectorNomalize(Point2f pt)
	{
		Point2f result;

		result.x = pt.x / sqrt(pow(pt.x, 2) + pow(pt.y, 2));
		result.y = pt.y / sqrt(pow(pt.x, 2) + pow(pt.y, 2));

		return result;
	}

	/*�Է¹��� ��ǥ�� (0,0) ���� ���̸� ���ϴ� �Լ�
	@param pt : ���̸� ���� ��ǥ
	*/
	float VectorLength(Point2f pt)
	{
		float dist = sqrt(pow(pt.x, 2) + pow(pt.y, 2));

		return dist;
	}

	/*������ �� ���̰��� angles ����Ʈ�� �����ϰ� �ߺ�üũ�ϴ� �Լ�
	������ �� ���̰��� anlges ����Ʈ�� �����ϰų� �ߺ��� ���� ������ �� ���� �ν� ī��Ʈ ���� �÷��ش�
	@param inner : ������
	@param length : ���̰�
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

	float lineLength = 0; // ���� ���� ����� ������ ����

	//anlges ����Ʈ���� ���� ���� ī��Ʈ�� ���� �������� ���̰��� ����ϴ� �Լ�
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