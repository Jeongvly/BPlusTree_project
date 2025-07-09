#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <queue>	
#include <iomanip>

using namespace std;

// Index entry (Non-leaf)
struct IndexEntry
{
	int key;
	int nextLevelBID;

	// key 기준 비교 (오름차순)
	bool operator<(const IndexEntry& e) const
	{
		return key < e.key;
	}
};

// Data Entry (Leaf)
struct DataEntry
{
	int key;
	int value;

	// Key 기준 비교 (오름차순)
	bool operator<(const DataEntry& e) const
	{
		return key < e.key;
	}
};

// None-leaf node 
struct NoneLeafNode
{
	int nextLevelBID;
	vector<IndexEntry> indexEntry;
};

// Leaf node
struct LeafNode
{
	vector<DataEntry> dataEntry;
	int nextBID;
};

// File header (B+tree 정보 저장) 
struct fileHeader
{
	int blockSize;	// 한 block의 크기 (byte)
	int rootBID;	// root node의 block ID
	int depth;		// tree의 깊이
};


// B+tree 클래스 
class BPTree
{
public:
	const char* fileName;	// B+tree 파일 이름
	fileHeader h;			// B+tree header 정보	
	int BID;				// 다음에 사용할 block ID
	int entryNum;			// 한 노드(block) 가용 가능 최대 entry 수

	// 생성자  
	BPTree(const char* fileName)
		:fileName{ fileName }, BID{ 2 }
	{
		// B+tree header 읽기
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		btree.read(reinterpret_cast<char*>(&h), sizeof(h));
		btree.close();

		// 한 block이 최대 수용 DataEntry, IndexEntry 수
		entryNum = (h.blockSize - 4) / 8;
	}

	// B+tree header 정보 파일에 update (첫 번째 block 사용)
	void headUpdate()
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		btree.write(reinterpret_cast<char*>(&h), sizeof(h));
		btree.close();
	}

	// B+tree 파일 생성 (최초 1회 실행)
	void create(const int& blockSize)
	{
		fstream btree{ fileName, ios::out | ios::binary };

		// B+tree header 초기화 및 파일 입력
		h = { blockSize, 1, 0 };
		btree.write(reinterpret_cast<const char*>(&h), sizeof(h));

		// Root Node 초기화 및 파일 입력
		DataEntry e{ 0, 0 };
		int nextBID = 0;
		int entryNum = (h.blockSize - 4) / 8;

		for (int i = 0; i < entryNum; ++i)
			btree.write(reinterpret_cast<const char*>(&e), sizeof(e));

		btree.write(reinterpret_cast<const char*>(&nextBID), sizeof(nextBID));

		btree.close();
	}

	// Leaf Node의 BID를 통해 해당 노드를 파일에서 읽어와 LeafNode 구조체로 return
	LeafNode getLeafNode(const int& inputBID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		// 입력 BID 위치로 이동
		btree.seekg(sizeof(h) + (inputBID - 1) * h.blockSize);

		LeafNode n;
		DataEntry e;

		// entryNum만큼 leaf node의 DataEntry를 읽어 LeafNode 초기화  
		for (int i = 0; i < entryNum; ++i)
		{
			btree.read(reinterpret_cast<char*>(&e), sizeof(e));
			if (e.key != 0)
				n.dataEntry.push_back(e);
		}
		// 마지막에는 leaf node의 next BID를 읽어 LeafNode 초기화
		btree.read(reinterpret_cast<char*>(&n.nextBID), sizeof(n.nextBID));

		btree.close();

		return n;
	}

	// None Leaf Node의 BID를 통해 해당 노드를 파일에서 읽어와 NoneLeafNode 구조체로 return
	NoneLeafNode getNoneLeafNode(const int& inputBID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// 입력 BID 위치로 이동
		btree.seekg(sizeof(h) + (inputBID - 1) * h.blockSize);
		NoneLeafNode n;
		IndexEntry e;

		// 먼저 none leaf node의 next level BID를 읽어 LeafNode 초기화
		btree.read(reinterpret_cast<char*>(&n.nextLevelBID), sizeof(n.nextLevelBID));

		// entryNum만큼 none leaf node의 DataEntry를 읽어 LeafNode 초기화
		for (int i = 0; i < entryNum; ++i)
		{
			btree.read(reinterpret_cast<char*>(&e), sizeof(e));
			if (e.key != 0)
				n.indexEntry.push_back(e);
		}
		btree.close();

		return n;
	}

	// LeafNode를 파일 내 해당 block에 업데이트
	void updateLeafNode(const int& inputBID, const LeafNode& n)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// 입력 BID 위치로 이동
		btree.seekp(sizeof(h) + (inputBID - 1) * h.blockSize);

		DataEntry emptyEntry{ 0, 0 };

		// entryNum만큼 leaf node의 DataEntry를 파일에 입력 (빈 공간은 빈 entry(0, 0)로 입력)
		for (int i = 0; i < entryNum; ++i)
		{
			if (i < n.dataEntry.size())
			{
				btree.write(reinterpret_cast<const char*>(&n.dataEntry[i]), sizeof(n.dataEntry[i]));
			}
			else
				btree.write(reinterpret_cast<const char*>(&emptyEntry), sizeof(emptyEntry));
		}

		// 마지막에 leaf node의 next BID를 파일에 입력
		btree.write(reinterpret_cast<const char*>(&n.nextBID), sizeof(n.nextBID));

		btree.close();
	}

	// NoneLeafNode를 파일 내 해당 block에 업데이트
	void updateNoneLeafNode(const int& inputBID, const NoneLeafNode& n)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// 입력 BID 위치로 이동
		btree.seekp(sizeof(h) + (inputBID - 1) * h.blockSize);

		IndexEntry emptyEntry{ 0, 0 };

		// 먼저 none leaf node의 next level BID를 파일에 입력
		btree.write(reinterpret_cast<const char*>(&n.nextLevelBID), sizeof(n.nextLevelBID));

		// entryNum만큼 none leaf node의 IndexEntry를 파일에 입력 (빈 공간은 빈 entry(0, 0)로 입력)
		for (int i = 0; i < entryNum; ++i)
		{
			if (i < n.indexEntry.size())
				btree.write(reinterpret_cast<const char*>(&n.indexEntry[i]), sizeof(n.indexEntry[i]));
			else
				btree.write(reinterpret_cast<const char*>(&emptyEntry), sizeof(emptyEntry));
		}

		btree.close();
	}

	// Leaf node를 분할하고 상위로 보낼 IndexEntry return
	IndexEntry splitLeaf(const DataEntry& s_e, const int& s_BID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// 분할전, 해당 입력 entry 삽입 후 정렬 
		LeafNode n = getLeafNode(s_BID);
		n.dataEntry.push_back(s_e);
		sort(n.dataEntry.begin(), n.dataEntry.end());

		IndexEntry up;
		DataEntry empty{ 0, 0 };
		LeafNode left;
		LeafNode right;

		int mid = (entryNum + 1) / 2;	// split 기준 index
		right.nextBID = n.nextBID;		// right node의 nextBID 연결 유지
		left.nextBID = BID;				// left node가 새 node를 가르킴

		up.key = n.dataEntry[mid].key;	// 부모에게 전달할 index entry의 key
		up.nextLevelBID = BID;			// 부모에게 전달할 index entry의 next level BID (right node의 BID)

		// 분할 수행 (홀수 분할일때, right node의 entry 수가 하나 더 많음)
		for (int i = 0; i < entryNum + 1; ++i)
		{
			if (i < mid)
				left.dataEntry.push_back(n.dataEntry[i]);
			else
			{
				right.dataEntry.push_back(n.dataEntry[i]);
			}
		}

		// left, right으로 split된 leaf node를 파일에 업데이트
		updateLeafNode(s_BID, left);
		updateLeafNode(BID, right);

		BID += 1;

		btree.close();
		return up;
	}

	// None leaf node를 분할하고 상위로 보낼 IndexEntry return
	IndexEntry splitNoneLeaf(const IndexEntry& s_e, const int& s_BID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// 분할전, 해당 입력 entry 삽입 후 정렬 
		NoneLeafNode n = getNoneLeafNode(s_BID);
		n.indexEntry.push_back(s_e);
		sort(n.indexEntry.begin(), n.indexEntry.end());

		IndexEntry up;
		IndexEntry empty{ 0, 0 };
		NoneLeafNode left;
		NoneLeafNode right;

		int mid = (entryNum) / 2;								// split 기준 index
		left.nextLevelBID = n.nextLevelBID;						// left leaf node의 next level BID 연결 유지
		right.nextLevelBID = n.indexEntry[mid].nextLevelBID;	// right leaf node의 next level BID 연결 유지
		up.key = n.indexEntry[mid].key;							// 부모에게 전달할 index entry의 key
		up.nextLevelBID = BID;									// 부모에게 전달할 index entry의 next level BID (right node의 BID)

		// 분할 수행 (홀수 분할일때, right node의 entry 수가 하나 더 많음)
		for (int i = 0; i < entryNum + 1; ++i)
		{
			if (i < mid)
				left.indexEntry.push_back(n.indexEntry[i]);

			if (i > mid)
				right.indexEntry.push_back(n.indexEntry[i]);
		}

		// left, right으로 split된 none leaf node를 파일에 업데이트
		updateNoneLeafNode(s_BID, left);
		updateNoneLeafNode(BID, right);

		BID += 1;
		btree.close();

		return up;
	}

	// Node 삽입 : 입력 파일에서 (key, value)를 읽고 insert 반복
	void insertFile(const char* inputFile)
	{
		fstream input{ inputFile, ios::in };
		string line;

		// 한 줄씩 (key, value) 읽기
		while (getline(input, line))
		{
			stringstream ss{ line };
			int key, value;
			char comma;
			ss >> key >> comma >> value;
			// (key, value) B+tree 파일에 입력
			insert({ key, value }, h.rootBID, 0);
		}
		headUpdate();
		input.close();
	}

	// 입력된 Data entry를 B+tree에 삽입하는 함수(재귀)
	// 입력 depth를 leaf node & none leaf node 판단 
	IndexEntry insert(const DataEntry& input_e, const int& inputBID, const int& input_depth)
	{

		// Leaf Node 도달
		if (input_depth == h.depth)
		{
			LeafNode n = getLeafNode(inputBID);

			// Node가 가득 찼을 경우 (split 필요) 
			if (n.dataEntry.size() == entryNum)
			{
				// Leaf node가 root node일 경우, root 자체를 분할
				if (input_depth == 0)
				{
					IndexEntry r = splitLeaf(input_e, inputBID);
					NoneLeafNode newRoot;
					newRoot.indexEntry.push_back(r);
					newRoot.nextLevelBID = inputBID;

					updateNoneLeafNode(BID, newRoot);

					h.rootBID = BID;
					BID += 1;
					h.depth += 1;

					return { 0, 0 };
				}
				// Leaf node가 root node가 아닐 경우, 상위로 IndexEntry(새로 생긴 node의 front entry) 전달
				else
				{
					return splitLeaf(input_e, inputBID);
				}

			}
			// Leaf node가 가득 차지 않은 경우
			else
			{
				// node에 entry 삽입 후 정렬하여 업데이트
				n.dataEntry.push_back(input_e);
				sort(n.dataEntry.begin(), n.dataEntry.end());

				updateLeafNode(inputBID, n);
				return { 0, 0 };
			}
		}
		// Leaf node가 아닌 경우 (inner node)
		else
		{
			NoneLeafNode n = getNoneLeafNode(inputBID);

			// 입력 entry가 포함되는 자식 노드 선택
			int nextLevelBID = n.nextLevelBID;
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (input_e.key < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}

			// 재귀적으로 자식 노드에 삽입
			IndexEntry r_e = insert(input_e, nextLevelBID, input_depth + 1);

			// 추가할 Entry 없는 경우 (자식 노드가 split 하지 않은 경우)
			if (r_e.key == 0)
			{
				return { 0, 0 };
			}

			// 추가할 Entry 있는 경우 (자식 노드가 split한 경우)
			else
			{
				NoneLeafNode n = getNoneLeafNode(inputBID);

				// Inner Node(none leaf node)가 가득 찼을 경우 (split 필요) 
				if (n.indexEntry.size() == entryNum)
				{
					// Inner Node(none leaf node)가 root node일 경우, root 자체를 분할
					if (input_depth == 0)
					{
						IndexEntry r = splitNoneLeaf(r_e, inputBID);

						NoneLeafNode n;
						n.indexEntry.push_back(r);
						n.nextLevelBID = inputBID;

						updateNoneLeafNode(BID, n);

						h.rootBID = BID;
						BID += 1;
						h.depth += 1;

						return { 0, 0 };
					}
					// Inner node(none leaf node)가 root node가 아닐 경우, 상위로 IndexEntry 전달
					else
					{
						return splitNoneLeaf(r_e, inputBID);
					}
				}
				// Inner node(none leaf node)가 가득 차지 않은 경우
				else
				{
					// node에 entry 삽입 후 정렬하여 업데이트
					n.indexEntry.push_back(r_e);
					sort(n.indexEntry.begin(), n.indexEntry.end());

					updateNoneLeafNode(inputBID, n);
					return { 0, 0 };
				}
			}
		}
	}

	// 특정 key가 있는 Block을 찾아 해당 value를 return하는 (재귀)함수
	int getValue(const int& inputKey, const int& inputBID, const int& inputDepth)
	{
		// Leaf node에 도달한 경우
		if (h.depth == inputDepth)
		{
			LeafNode n = getLeafNode(inputBID);
			for (int i = 0; i < n.dataEntry.size(); ++i)
			{
				// key를 찾은 경우 해당 value return
				if (inputKey == n.dataEntry[i].key)
				{
					return n.dataEntry[i].value;
				}
			}
		}
		// Inner node(none leaf node)인 경우 -> next level BID로 재탐색
		else
		{
			NoneLeafNode n = getNoneLeafNode(inputBID);
			int nextLevelBID = n.nextLevelBID;

			// key를 포함하는 자식 node BID 선택
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (inputKey < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}
			// next level BID로 재탐색
			return getValue(inputKey, nextLevelBID, inputDepth + 1);
		}

		// key가 없는 경우
		return 0;
	}
	// search file (검색할 keys)을 읽고 결과를 output file에 저장
	void pointSearch(const char* searchFile, const char* outputFile)
	{
		fstream search{ searchFile, ios::in };
		fstream output{ outputFile, ios::out };

		int key;
		int value;

		// search file에서 key를 한 줄씩 읽어 해당 value를 탐색하여 output file에 저장
		while (search >> key)
		{
			value = getValue(key, h.rootBID, 0);
			output << key << "," << value << endl;
		}

		search.close();
		output.close();
	}

	// 특정 key가 포함될 수 있는 Block을 찾아 BID를 return하는 (재귀)함수
	int getBID(const int& inputKey, const int& input_BID, const int& input_depth)
	{
		// Leaf node에 도달한 경우
		if (h.depth == input_depth)
		{
			return input_BID;
		}
		// Inner node(none leaf node)인 경우 -> next level BID로 재탐색
		else
		{
			NoneLeafNode n = getNoneLeafNode(input_BID);
			int nextLevelBID = n.nextLevelBID;

			// key를 포함하는 자식 node BID 선택
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (inputKey < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}

			// next level BID로 재탐색
			return getBID(inputKey, nextLevelBID, input_depth + 1);
		}
	}

	// start range ~ end range 사이의 entry를 output file에 저장하는 함수
	void rangeSearchSub(const int& startRange, const int& endRange, const int& inputBID, const char* outputFile)
	{
		fstream output{ outputFile, ios::out | ios::app };
		LeafNode n = getLeafNode(inputBID);
		bool flag = true;		// end range를 넘지 않았는지 판단하는 flag

		// 현재 leaf node의 모든 entry 순회
		for (int i = 0; i < n.dataEntry.size(); ++i)
		{
			// 범위에 포함되는 경우 output file에 저장
			if (n.dataEntry[i].key >= startRange && n.dataEntry[i].key <= endRange)
				output << left << setw(10) << to_string(n.dataEntry[i].key) + "," + to_string(n.dataEntry[i].value);

			// 범위를 넘은 경우 더 이상 탐색하지 않고 종료
			else if (endRange < n.dataEntry[i].key)
			{
				flag = false;
				break;
			}
		}
		output.close();

		// 다음 leaf node가 존재하고 end range를 넘지 않은 경우 재귀호출 
		if (flag == true && n.nextBID != 0)
			rangeSearchSub(startRange, endRange, n.nextBID, outputFile);
	}

	// range search의 시작과 끝 범위를 입력 받아 결과를 output file에 저장 
	void rangeSearch(const char* searchFile, const char* outputFile)
	{
		fstream search{ searchFile, ios::in };
		fstream output{ outputFile, ios::out | ios::app };

		string line;

		// search file에서 한 줄씩 범위 읽음 e.g.(100,100)
		while (getline(search, line))
		{
			stringstream ss{ line };
			int startRange, endRange;
			char r;

			// 쉼표 구분자를 기준으로 분할
			ss >> startRange >> r >> endRange;
			// 시작 범위에 해당하는 leaf node BID 탐색
			int s_BID = getBID(startRange, h.rootBID, 0);
			// 해당 BID부터 시작하여 range search 수행
			rangeSearchSub(startRange, endRange, s_BID, outputFile);

			output << endl;
		}

		search.close();
		output.close();
	}


	// B+tree의 특정 level에 존재하는 node들의 BID가 주어진 qeueu p를 입력 받아,
	// 해당 node들의 key 값을 출력 파일에 입력하고, next level BID를 queue q에 저장하여 재귀 호출
	void printSub(const char* ouputFile, queue<int>& p, const int& inputDepth)
	{
		fstream btree{ fileName, ios::in | ios::binary };
		fstream output{ ouputFile, ios::out | ios::app };
		queue<int> q;		// next level BID를 저장할 큐

		// 현재 level 정보 출력
		output << "<" << inputDepth << ">" << "\n" << endl;

		// 현재 level의 모든 node를 순회
		while (!p.empty())
		{
			int bid = p.front();
			// 현재 node가 leaf node인 경우
			if (inputDepth == h.depth)
			{
				LeafNode n = getLeafNode(bid);
				for (int i = 0; i < n.dataEntry.size(); ++i)
				{
					output << n.dataEntry[i].key << ",";
				}
				p.pop();
			}
			// 현재 node가 inner node(none leaf node)인 경우
			else
			{
				NoneLeafNode n = getNoneLeafNode(bid);

				// next level BID push
				q.push(n.nextLevelBID);
				for (int i = 0; i < n.indexEntry.size(); ++i)
				{
					output << n.indexEntry[i].key << ",";
					q.push(n.indexEntry[i].nextLevelBID);
				}
				p.pop();

			}
		}
		output << "\n" << endl;
		output.close();
		btree.close();

		// 다음 level node가 존재하면 재귀 호출 (level 1까지만 출력하므로 생략(주석 처리))
		if (!q.empty())
		{
			//printSub(ouputFile, q, inputDepth + 1);
		}
	}

	// 출력 파일에 B+tree 구조 입력 함수 
	void print(const char* outputFile)
	{
		fstream btree{ fileName, ios::in | ios::binary };
		fstream output{ outputFile, ios::out };

		queue<int> q;		// root node의 next level BID를 저장할 queue
		output << "<0>" << "\n" << endl;

		// B+tree의 depth가 0인 경우 (root node만 파일에 입력)
		if (h.depth == 0)
		{
			LeafNode n = getLeafNode(h.rootBID);

			for (int i = 0; i < n.dataEntry.size(); ++i)
			{
				output << n.dataEntry[i].key << ",";
			}
		}
		// B+tree의 depth가 1 이상인 경우
		else
		{
			// root node의 next level BID를 q에 push
			NoneLeafNode n = getNoneLeafNode(h.rootBID);
			q.push(n.nextLevelBID);

			// root node 파일에 입력
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				output << n.indexEntry[i].key << ",";
				q.push(n.indexEntry[i].nextLevelBID);
			}
			output << "\n" << endl;

			// next level BID 출력 함수 호출
			printSub(outputFile, q, 1);
		}
	}
};

int main(int agrc, char* argv[])
{
	char command = argv[1][0];
	const char* filename = argv[2];

	BPTree* myBtree = new BPTree(filename);

	switch (command)
	{
	case 'c':
		// index file 생성
		myBtree->create(atoi(argv[3]));
		break;
	case 'i':
		// records 삽입 (from [records data file], ex) records.txt) 
		myBtree->insertFile(argv[3]);
		break;
	case 's':
		// keys 탐색 in [input file] 및 결과 출력 to [output file] 
		myBtree->pointSearch(argv[3], argv[4]);
		break;
	case 'r':
		// keys 범위 탐색 in [input file] 및 결과 출력 to [output file] 
		myBtree->rangeSearch(argv[3], argv[4]);
		break;
	case 'p':
		// B+-Tree 구조 출력 to [output file] 
		myBtree->print(argv[3]);
		break;
	}
}