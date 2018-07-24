# FindPeaksAndRotation
Find Peaks( the Max value in a part of image area) and if input-image is rotated, It's recovered.
 
본래 포렌식 워터마킹 작업 중 일부분입니다.
 
본래 사용될 이미지는 2차 웨이블릿 변환으로 HH2 영역에 peak값들을 삽입 후 다시 역변환한 이미지입니다.
 
FindPeaks 클래스에서 받을 매개변수 src는
 
peak값들을 추출할 이미지를 A,
 
A를 3x3블럭 조건으로 위너필터를 적용한 이미지를 B라고 하면
 
A와 B의 차이로 생성된 이미지 C가 생성되며
 
C를 자기상관함수(auto correlation function)를 사용하여 peak값들을 선명하게 만든 결과 이미지 D를 받게됩니다.

본 프로젝트구성에서는 본래 들어가야할 이미지 대신 다른 처리과정이 없는 이미지에 흰색의 peak점들을 넣어 테스트하게됩니다.
즉, 특정 각도에서는 제대로 동작하지 않을 가능성이 높습니다.

<입력된 이미지>

![dodo_peaks_added_rotate_20](/FindPeaksAndRotation(VS2017)/FindPeaksAndRotation(VS2017)/image/result/dodo_peaks_added_rotate_20.png)


<입력된 이미지에서 추출된 peak값들 중 조건에 맞는 직선들 추출>

![dodo_peaks_added_rotate_20_peaks_lines](/FindPeaksAndRotation(VS2017)/FindPeaksAndRotation(VS2017)/image/result/dodo_peaks_added_rotate_20_peaks_lines.png)


<추출된 직선들의 각도를 토대로 기존 각도로 복구한 이미지>

![dodo_peaks_added_rotate_20_output](/FindPeaksAndRotation(VS2017)/FindPeaksAndRotation(VS2017)/image/result/dodo_peaks_added_rotate_20_output.png)
