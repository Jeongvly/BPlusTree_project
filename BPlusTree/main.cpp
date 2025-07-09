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

	// key ���� �� (��������)
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

	// Key ���� �� (��������)
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

// File header (B+tree ���� ����) 
struct fileHeader
{
	int blockSize;	// �� block�� ũ�� (byte)
	int rootBID;	// root node�� block ID
	int depth;		// tree�� ����
};


// B+tree Ŭ���� 
class BPTree
{
public:
	const char* fileName;	// B+tree ���� �̸�
	fileHeader h;			// B+tree header ����	
	int BID;				// ������ ����� block ID
	int entryNum;			// �� ���(block) ���� ���� �ִ� entry ��

	// ������  
	BPTree(const char* fileName)
		:fileName{ fileName }, BID{ 2 }
	{
		// B+tree header �б�
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		btree.read(reinterpret_cast<char*>(&h), sizeof(h));
		btree.close();

		// �� block�� �ִ� ���� DataEntry, IndexEntry ��
		entryNum = (h.blockSize - 4) / 8;
	}

	// B+tree header ���� ���Ͽ� update (ù ��° block ���)
	void headUpdate()
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		btree.write(reinterpret_cast<char*>(&h), sizeof(h));
		btree.close();
	}

	// B+tree ���� ���� (���� 1ȸ ����)
	void create(const int& blockSize)
	{
		fstream btree{ fileName, ios::out | ios::binary };

		// B+tree header �ʱ�ȭ �� ���� �Է�
		h = { blockSize, 1, 0 };
		btree.write(reinterpret_cast<const char*>(&h), sizeof(h));

		// Root Node �ʱ�ȭ �� ���� �Է�
		DataEntry e{ 0, 0 };
		int nextBID = 0;
		int entryNum = (h.blockSize - 4) / 8;

		for (int i = 0; i < entryNum; ++i)
			btree.write(reinterpret_cast<const char*>(&e), sizeof(e));

		btree.write(reinterpret_cast<const char*>(&nextBID), sizeof(nextBID));

		btree.close();
	}

	// Leaf Node�� BID�� ���� �ش� ��带 ���Ͽ��� �о�� LeafNode ����ü�� return
	LeafNode getLeafNode(const int& inputBID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };
		// �Է� BID ��ġ�� �̵�
		btree.seekg(sizeof(h) + (inputBID - 1) * h.blockSize);

		LeafNode n;
		DataEntry e;

		// entryNum��ŭ leaf node�� DataEntry�� �о� LeafNode �ʱ�ȭ  
		for (int i = 0; i < entryNum; ++i)
		{
			btree.read(reinterpret_cast<char*>(&e), sizeof(e));
			if (e.key != 0)
				n.dataEntry.push_back(e);
		}
		// ���������� leaf node�� next BID�� �о� LeafNode �ʱ�ȭ
		btree.read(reinterpret_cast<char*>(&n.nextBID), sizeof(n.nextBID));

		btree.close();

		return n;
	}

	// None Leaf Node�� BID�� ���� �ش� ��带 ���Ͽ��� �о�� NoneLeafNode ����ü�� return
	NoneLeafNode getNoneLeafNode(const int& inputBID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// �Է� BID ��ġ�� �̵�
		btree.seekg(sizeof(h) + (inputBID - 1) * h.blockSize);
		NoneLeafNode n;
		IndexEntry e;

		// ���� none leaf node�� next level BID�� �о� LeafNode �ʱ�ȭ
		btree.read(reinterpret_cast<char*>(&n.nextLevelBID), sizeof(n.nextLevelBID));

		// entryNum��ŭ none leaf node�� DataEntry�� �о� LeafNode �ʱ�ȭ
		for (int i = 0; i < entryNum; ++i)
		{
			btree.read(reinterpret_cast<char*>(&e), sizeof(e));
			if (e.key != 0)
				n.indexEntry.push_back(e);
		}
		btree.close();

		return n;
	}

	// LeafNode�� ���� �� �ش� block�� ������Ʈ
	void updateLeafNode(const int& inputBID, const LeafNode& n)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// �Է� BID ��ġ�� �̵�
		btree.seekp(sizeof(h) + (inputBID - 1) * h.blockSize);

		DataEntry emptyEntry{ 0, 0 };

		// entryNum��ŭ leaf node�� DataEntry�� ���Ͽ� �Է� (�� ������ �� entry(0, 0)�� �Է�)
		for (int i = 0; i < entryNum; ++i)
		{
			if (i < n.dataEntry.size())
			{
				btree.write(reinterpret_cast<const char*>(&n.dataEntry[i]), sizeof(n.dataEntry[i]));
			}
			else
				btree.write(reinterpret_cast<const char*>(&emptyEntry), sizeof(emptyEntry));
		}

		// �������� leaf node�� next BID�� ���Ͽ� �Է�
		btree.write(reinterpret_cast<const char*>(&n.nextBID), sizeof(n.nextBID));

		btree.close();
	}

	// NoneLeafNode�� ���� �� �ش� block�� ������Ʈ
	void updateNoneLeafNode(const int& inputBID, const NoneLeafNode& n)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// �Է� BID ��ġ�� �̵�
		btree.seekp(sizeof(h) + (inputBID - 1) * h.blockSize);

		IndexEntry emptyEntry{ 0, 0 };

		// ���� none leaf node�� next level BID�� ���Ͽ� �Է�
		btree.write(reinterpret_cast<const char*>(&n.nextLevelBID), sizeof(n.nextLevelBID));

		// entryNum��ŭ none leaf node�� IndexEntry�� ���Ͽ� �Է� (�� ������ �� entry(0, 0)�� �Է�)
		for (int i = 0; i < entryNum; ++i)
		{
			if (i < n.indexEntry.size())
				btree.write(reinterpret_cast<const char*>(&n.indexEntry[i]), sizeof(n.indexEntry[i]));
			else
				btree.write(reinterpret_cast<const char*>(&emptyEntry), sizeof(emptyEntry));
		}

		btree.close();
	}

	// Leaf node�� �����ϰ� ������ ���� IndexEntry return
	IndexEntry splitLeaf(const DataEntry& s_e, const int& s_BID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// ������, �ش� �Է� entry ���� �� ���� 
		LeafNode n = getLeafNode(s_BID);
		n.dataEntry.push_back(s_e);
		sort(n.dataEntry.begin(), n.dataEntry.end());

		IndexEntry up;
		DataEntry empty{ 0, 0 };
		LeafNode left;
		LeafNode right;

		int mid = (entryNum + 1) / 2;	// split ���� index
		right.nextBID = n.nextBID;		// right node�� nextBID ���� ����
		left.nextBID = BID;				// left node�� �� node�� ����Ŵ

		up.key = n.dataEntry[mid].key;	// �θ𿡰� ������ index entry�� key
		up.nextLevelBID = BID;			// �θ𿡰� ������ index entry�� next level BID (right node�� BID)

		// ���� ���� (Ȧ�� �����϶�, right node�� entry ���� �ϳ� �� ����)
		for (int i = 0; i < entryNum + 1; ++i)
		{
			if (i < mid)
				left.dataEntry.push_back(n.dataEntry[i]);
			else
			{
				right.dataEntry.push_back(n.dataEntry[i]);
			}
		}

		// left, right���� split�� leaf node�� ���Ͽ� ������Ʈ
		updateLeafNode(s_BID, left);
		updateLeafNode(BID, right);

		BID += 1;

		btree.close();
		return up;
	}

	// None leaf node�� �����ϰ� ������ ���� IndexEntry return
	IndexEntry splitNoneLeaf(const IndexEntry& s_e, const int& s_BID)
	{
		fstream btree{ fileName, ios::in | ios::out | ios::binary };

		// ������, �ش� �Է� entry ���� �� ���� 
		NoneLeafNode n = getNoneLeafNode(s_BID);
		n.indexEntry.push_back(s_e);
		sort(n.indexEntry.begin(), n.indexEntry.end());

		IndexEntry up;
		IndexEntry empty{ 0, 0 };
		NoneLeafNode left;
		NoneLeafNode right;

		int mid = (entryNum) / 2;								// split ���� index
		left.nextLevelBID = n.nextLevelBID;						// left leaf node�� next level BID ���� ����
		right.nextLevelBID = n.indexEntry[mid].nextLevelBID;	// right leaf node�� next level BID ���� ����
		up.key = n.indexEntry[mid].key;							// �θ𿡰� ������ index entry�� key
		up.nextLevelBID = BID;									// �θ𿡰� ������ index entry�� next level BID (right node�� BID)

		// ���� ���� (Ȧ�� �����϶�, right node�� entry ���� �ϳ� �� ����)
		for (int i = 0; i < entryNum + 1; ++i)
		{
			if (i < mid)
				left.indexEntry.push_back(n.indexEntry[i]);

			if (i > mid)
				right.indexEntry.push_back(n.indexEntry[i]);
		}

		// left, right���� split�� none leaf node�� ���Ͽ� ������Ʈ
		updateNoneLeafNode(s_BID, left);
		updateNoneLeafNode(BID, right);

		BID += 1;
		btree.close();

		return up;
	}

	// Node ���� : �Է� ���Ͽ��� (key, value)�� �а� insert �ݺ�
	void insertFile(const char* inputFile)
	{
		fstream input{ inputFile, ios::in };
		string line;

		// �� �پ� (key, value) �б�
		while (getline(input, line))
		{
			stringstream ss{ line };
			int key, value;
			char comma;
			ss >> key >> comma >> value;
			// (key, value) B+tree ���Ͽ� �Է�
			insert({ key, value }, h.rootBID, 0);
		}
		headUpdate();
		input.close();
	}

	// �Էµ� Data entry�� B+tree�� �����ϴ� �Լ�(���)
	// �Է� depth�� leaf node & none leaf node �Ǵ� 
	IndexEntry insert(const DataEntry& input_e, const int& inputBID, const int& input_depth)
	{

		// Leaf Node ����
		if (input_depth == h.depth)
		{
			LeafNode n = getLeafNode(inputBID);

			// Node�� ���� á�� ��� (split �ʿ�) 
			if (n.dataEntry.size() == entryNum)
			{
				// Leaf node�� root node�� ���, root ��ü�� ����
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
				// Leaf node�� root node�� �ƴ� ���, ������ IndexEntry(���� ���� node�� front entry) ����
				else
				{
					return splitLeaf(input_e, inputBID);
				}

			}
			// Leaf node�� ���� ���� ���� ���
			else
			{
				// node�� entry ���� �� �����Ͽ� ������Ʈ
				n.dataEntry.push_back(input_e);
				sort(n.dataEntry.begin(), n.dataEntry.end());

				updateLeafNode(inputBID, n);
				return { 0, 0 };
			}
		}
		// Leaf node�� �ƴ� ��� (inner node)
		else
		{
			NoneLeafNode n = getNoneLeafNode(inputBID);

			// �Է� entry�� ���ԵǴ� �ڽ� ��� ����
			int nextLevelBID = n.nextLevelBID;
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (input_e.key < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}

			// ��������� �ڽ� ��忡 ����
			IndexEntry r_e = insert(input_e, nextLevelBID, input_depth + 1);

			// �߰��� Entry ���� ��� (�ڽ� ��尡 split ���� ���� ���)
			if (r_e.key == 0)
			{
				return { 0, 0 };
			}

			// �߰��� Entry �ִ� ��� (�ڽ� ��尡 split�� ���)
			else
			{
				NoneLeafNode n = getNoneLeafNode(inputBID);

				// Inner Node(none leaf node)�� ���� á�� ��� (split �ʿ�) 
				if (n.indexEntry.size() == entryNum)
				{
					// Inner Node(none leaf node)�� root node�� ���, root ��ü�� ����
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
					// Inner node(none leaf node)�� root node�� �ƴ� ���, ������ IndexEntry ����
					else
					{
						return splitNoneLeaf(r_e, inputBID);
					}
				}
				// Inner node(none leaf node)�� ���� ���� ���� ���
				else
				{
					// node�� entry ���� �� �����Ͽ� ������Ʈ
					n.indexEntry.push_back(r_e);
					sort(n.indexEntry.begin(), n.indexEntry.end());

					updateNoneLeafNode(inputBID, n);
					return { 0, 0 };
				}
			}
		}
	}

	// Ư�� key�� �ִ� Block�� ã�� �ش� value�� return�ϴ� (���)�Լ�
	int getValue(const int& inputKey, const int& inputBID, const int& inputDepth)
	{
		// Leaf node�� ������ ���
		if (h.depth == inputDepth)
		{
			LeafNode n = getLeafNode(inputBID);
			for (int i = 0; i < n.dataEntry.size(); ++i)
			{
				// key�� ã�� ��� �ش� value return
				if (inputKey == n.dataEntry[i].key)
				{
					return n.dataEntry[i].value;
				}
			}
		}
		// Inner node(none leaf node)�� ��� -> next level BID�� ��Ž��
		else
		{
			NoneLeafNode n = getNoneLeafNode(inputBID);
			int nextLevelBID = n.nextLevelBID;

			// key�� �����ϴ� �ڽ� node BID ����
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (inputKey < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}
			// next level BID�� ��Ž��
			return getValue(inputKey, nextLevelBID, inputDepth + 1);
		}

		// key�� ���� ���
		return 0;
	}
	// search file (�˻��� keys)�� �а� ����� output file�� ����
	void pointSearch(const char* searchFile, const char* outputFile)
	{
		fstream search{ searchFile, ios::in };
		fstream output{ outputFile, ios::out };

		int key;
		int value;

		// search file���� key�� �� �پ� �о� �ش� value�� Ž���Ͽ� output file�� ����
		while (search >> key)
		{
			value = getValue(key, h.rootBID, 0);
			output << key << "," << value << endl;
		}

		search.close();
		output.close();
	}

	// Ư�� key�� ���Ե� �� �ִ� Block�� ã�� BID�� return�ϴ� (���)�Լ�
	int getBID(const int& inputKey, const int& input_BID, const int& input_depth)
	{
		// Leaf node�� ������ ���
		if (h.depth == input_depth)
		{
			return input_BID;
		}
		// Inner node(none leaf node)�� ��� -> next level BID�� ��Ž��
		else
		{
			NoneLeafNode n = getNoneLeafNode(input_BID);
			int nextLevelBID = n.nextLevelBID;

			// key�� �����ϴ� �ڽ� node BID ����
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				if (inputKey < n.indexEntry[i].key)
					break;
				else
					nextLevelBID = n.indexEntry[i].nextLevelBID;
			}

			// next level BID�� ��Ž��
			return getBID(inputKey, nextLevelBID, input_depth + 1);
		}
	}

	// start range ~ end range ������ entry�� output file�� �����ϴ� �Լ�
	void rangeSearchSub(const int& startRange, const int& endRange, const int& inputBID, const char* outputFile)
	{
		fstream output{ outputFile, ios::out | ios::app };
		LeafNode n = getLeafNode(inputBID);
		bool flag = true;		// end range�� ���� �ʾҴ��� �Ǵ��ϴ� flag

		// ���� leaf node�� ��� entry ��ȸ
		for (int i = 0; i < n.dataEntry.size(); ++i)
		{
			// ������ ���ԵǴ� ��� output file�� ����
			if (n.dataEntry[i].key >= startRange && n.dataEntry[i].key <= endRange)
				output << left << setw(10) << to_string(n.dataEntry[i].key) + "," + to_string(n.dataEntry[i].value);

			// ������ ���� ��� �� �̻� Ž������ �ʰ� ����
			else if (endRange < n.dataEntry[i].key)
			{
				flag = false;
				break;
			}
		}
		output.close();

		// ���� leaf node�� �����ϰ� end range�� ���� ���� ��� ���ȣ�� 
		if (flag == true && n.nextBID != 0)
			rangeSearchSub(startRange, endRange, n.nextBID, outputFile);
	}

	// range search�� ���۰� �� ������ �Է� �޾� ����� output file�� ���� 
	void rangeSearch(const char* searchFile, const char* outputFile)
	{
		fstream search{ searchFile, ios::in };
		fstream output{ outputFile, ios::out | ios::app };

		string line;

		// search file���� �� �پ� ���� ���� e.g.(100,100)
		while (getline(search, line))
		{
			stringstream ss{ line };
			int startRange, endRange;
			char r;

			// ��ǥ �����ڸ� �������� ����
			ss >> startRange >> r >> endRange;
			// ���� ������ �ش��ϴ� leaf node BID Ž��
			int s_BID = getBID(startRange, h.rootBID, 0);
			// �ش� BID���� �����Ͽ� range search ����
			rangeSearchSub(startRange, endRange, s_BID, outputFile);

			output << endl;
		}

		search.close();
		output.close();
	}


	// B+tree�� Ư�� level�� �����ϴ� node���� BID�� �־��� qeueu p�� �Է� �޾�,
	// �ش� node���� key ���� ��� ���Ͽ� �Է��ϰ�, next level BID�� queue q�� �����Ͽ� ��� ȣ��
	void printSub(const char* ouputFile, queue<int>& p, const int& inputDepth)
	{
		fstream btree{ fileName, ios::in | ios::binary };
		fstream output{ ouputFile, ios::out | ios::app };
		queue<int> q;		// next level BID�� ������ ť

		// ���� level ���� ���
		output << "<" << inputDepth << ">" << "\n" << endl;

		// ���� level�� ��� node�� ��ȸ
		while (!p.empty())
		{
			int bid = p.front();
			// ���� node�� leaf node�� ���
			if (inputDepth == h.depth)
			{
				LeafNode n = getLeafNode(bid);
				for (int i = 0; i < n.dataEntry.size(); ++i)
				{
					output << n.dataEntry[i].key << ",";
				}
				p.pop();
			}
			// ���� node�� inner node(none leaf node)�� ���
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

		// ���� level node�� �����ϸ� ��� ȣ�� (level 1������ ����ϹǷ� ����(�ּ� ó��))
		if (!q.empty())
		{
			//printSub(ouputFile, q, inputDepth + 1);
		}
	}

	// ��� ���Ͽ� B+tree ���� �Է� �Լ� 
	void print(const char* outputFile)
	{
		fstream btree{ fileName, ios::in | ios::binary };
		fstream output{ outputFile, ios::out };

		queue<int> q;		// root node�� next level BID�� ������ queue
		output << "<0>" << "\n" << endl;

		// B+tree�� depth�� 0�� ��� (root node�� ���Ͽ� �Է�)
		if (h.depth == 0)
		{
			LeafNode n = getLeafNode(h.rootBID);

			for (int i = 0; i < n.dataEntry.size(); ++i)
			{
				output << n.dataEntry[i].key << ",";
			}
		}
		// B+tree�� depth�� 1 �̻��� ���
		else
		{
			// root node�� next level BID�� q�� push
			NoneLeafNode n = getNoneLeafNode(h.rootBID);
			q.push(n.nextLevelBID);

			// root node ���Ͽ� �Է�
			for (int i = 0; i < n.indexEntry.size(); ++i)
			{
				output << n.indexEntry[i].key << ",";
				q.push(n.indexEntry[i].nextLevelBID);
			}
			output << "\n" << endl;

			// next level BID ��� �Լ� ȣ��
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
		// index file ����
		myBtree->create(atoi(argv[3]));
		break;
	case 'i':
		// records ���� (from [records data file], ex) records.txt) 
		myBtree->insertFile(argv[3]);
		break;
	case 's':
		// keys Ž�� in [input file] �� ��� ��� to [output file] 
		myBtree->pointSearch(argv[3], argv[4]);
		break;
	case 'r':
		// keys ���� Ž�� in [input file] �� ��� ��� to [output file] 
		myBtree->rangeSearch(argv[3], argv[4]);
		break;
	case 'p':
		// B+-Tree ���� ��� to [output file] 
		myBtree->print(argv[3]);
		break;
	}
}