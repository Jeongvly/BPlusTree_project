# 📚 B+ Tree 기반 파일 구조

## 🔧 프로젝트 목적

B+tree를 기반으로 학생 정보를 **효율적으로 저장**하는 파일 구조를 구현합니다.  

## 📌 주요 자료구조 및 구성

### 1. 사용 라이브러리

- `<vector>`: Node 내 entry 저장을 위한 배열
- `<fstream>`: 파일 I/O 및 B+ Tree를 binary로 저장/로딩
- `<algorithm>`: `sort()`를 사용한 entry 정렬
- `<sstream>`: 문자열에서 key, value 추출
- `<string>`: 문자열 처리
- `<queue>`: B+ Tree 계층 순회에 사용
- `<iomanip>`: 출력 형식 정렬

---

### 2. Class: `BPlusTree`

#### 멤버 변수

- `filename`: B+ Tree가 저장될 파일 이름
- `h`: `fileHeader` 구조체 (blockSize, rootBID, depth 정보 포함)
- `BID`: 다음에 할당될 block ID
- `entryNum`: 한 노드에 들어갈 수 있는 최대 entry 수

#### 주요 함수

##### 헤더 갱신
- `headUpdate()`: 파일 헤더 정보(blockSize, root BID, depth) 갱신

##### 인덱스 생성
- `create()`: 초기 leaf node 생성 및 header 작성
- `getLeafNode()`, `getNoneLeafNode()`: BID에 따른 노드 불러오기
- `updateLeafNode()`, `updateNoneLeafNode()`: 노드 저장
- `splitLeaf()`, `splitNoneLeaf()`: 노드 분할 및 index entry 반환

##### 삽입
- `insertFile()`: 파일에서 (key, value) 읽어 삽입 수행
- `insert()`: 단일 (key, value) 삽입 (재귀 삽입)

##### 검색
- `pointSearch()`: 특정 key들에 대해 value 검색 결과 출력
- `getValue()`: 특정 key에 대한 value 반환

##### 범위 검색
- `rangeSearch()`: 범위에 해당하는 value 출력
- `getBID()`: 시작 leaf node BID 반환
- `rangeSearchSub()`: leaf node부터 범위 검색 수행

##### 트리 출력
- `print()`: level-0, level-1의 노드 key 출력
- `printSub()`: 각 level별 노드 key 출력

---

### 3. 기타 구성 요소

- `struct IndexEntry`: (key, nextLevelBID) 저장
- `struct DataEntry`: (key, value) 저장
- `struct NoneLeafNode`: inner node 구성 (왼쪽 자식 BID + IndexEntry 배열)
- `struct LeafNode`: leaf node 구성 (DataEntry 배열 + nextBID)
- `struct fileHeader`: 전체 B+ Tree 정보 저장

---

### 4. Main()

- 사용자로부터 파일명 입력 → `BPlusTree` 객체 생성
- 명령어 입력에 따라 다음 함수 실행:
  - `'c'`: `create()`
  - `'i'`: `insertFile()`
  - `'s'`: `pointSearch()`
  - `'r'`: `rangeSearch()`
  - `'p'`: `print()`

---

## 📌 개발 환경

- **CPU**: Intel  
- **OS**: Windows 11  
- **Language**: C++14  
- **IDE**: Visual Studio 2022

---

## 📌 프로젝트 후기

이번 프로젝트는 수업 시간에 배운 B+ Tree 이론을 바탕으로, **파일 기반 인덱스 구조**를 직접 구현하는 경험이었습니다.  
초기에는 파일 입출력과 디스크 구조에 익숙하지 않아 어려움이 많았지만, 객체지향프로그래밍2 수업의 지식을 바탕으로 차근차근 구현해 나갈 수 있었습니다.

디버깅 시에는 메모리 구조를 직접 확인할 수 없었기 때문에, **중간 출력 로그를 통해 디스크 I/O를 검증**하며 문제를 해결해 나갔습니다.

특히 `insertion`, `point search`, `range search` 등을 하나씩 구현하며 성취감을 느낄 수 있었고, `print()` 기능을 통해 전체 트리 구조를 확인하던 중 **마지막 값이 중복되는 문제**를 발견했습니다. 원인은 입력 파일의 **마지막 줄의 줄바꿈 문자로 인한 빈 entry 삽입**이었습니다.

이 경험을 통해 사소한 파일 처리 실수가 전체 구조에 큰 영향을 줄 수 있다는 점을 배웠습니다.

무엇보다도, 단순한 메모리 기반 알고리즘이 아닌 **디스크 기반 자료구조의 복잡성과 효율성**, 그리고 **B+ Tree의 split 과정의 실제 동작 원리**를 깊이 있게 이해할 수 있는 계기가 되었습니다.
