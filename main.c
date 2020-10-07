#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define BLOCK_SIZE 0x80000

#define windowSize 60
#define bufferSize 40
#define arraySize bufferSize + windowSize
typedef enum { false, true } bool;

typedef struct huffman_node {
	int weight;
	short lch, rch;
} huffman_t;

typedef struct huffman_control {
	int blocklen;
	int codelen;
	int treesize;
} huffman_header_t;

typedef struct huffman_code {
	unsigned char code[24];
	int codelen;
} huffman_code_t;

int huffman_initialize_tree(unsigned char *block, huffman_t *tree);
int huffman_deflate(huffman_t *tree, const unsigned char *input, size_t length, unsigned char *output);
void deflate(FILE *infile, FILE *outfile);
long fsize (FILE *in);
void compress(FILE *fileInput,FILE *fileOutput);
int findMatch(unsigned char window[], unsigned char str[], int strLen);
int main()
{
    FILE *giris, *giris1, *deflatee, *lz77e;

    giris1 = fopen("giris1.txt", "rb");
	giris = fopen("giris.txt", "rb");
	deflatee = fopen("deflate.txt", "wb");
    lz77e = fopen("lz77.txt", "wb");



    printf("LZ77 ALGORITMASI:\n\n");
    int giris_d = fsize(giris);
    printf("Giris dosyasinin boyutu: %d KB\n", giris_d);
    clock_t begin = clock();
    compress(giris,lz77e);
    clock_t end = clock();
    printf("Execution time: ");
    printf("%f", ((double) (end - begin) / CLOCKS_PER_SEC));
    printf(" [seconds]\n");
    int lz77_size = fsize(lz77e);
    printf("Cikis dosyasinin boyutu: %d KB \n\n",lz77_size);


    printf("DEFLATE ALGORITMASI:\n\n");
    int giris_1 = fsize(giris1);
    printf("Giris dosyasinin boyutu: %d KB \n", giris_1);
    clock_t deflate_begin = clock();
	deflate(giris1,deflatee);
    clock_t deflate_end = clock();
    printf("Execution time: ");
    printf("%f", ((double) (deflate_end - deflate_begin) / CLOCKS_PER_SEC));
    printf(" [seconds]\n");
    int deflate_size = fsize(deflatee);
    printf("Cikis dosyasinin boyutu: %d KB \n", deflate_size);

	fclose(giris);
    fclose(giris1);
	fclose(lz77e);
	fclose(deflate);
    return 0;
}
static int huffman_next_free_slot(huffman_t *tree, int *size)
{
	int i;
	for(i = 0; i < 256; i++)
		if(tree[i].weight == -2)
			return i;
	return (*size)++;
}

static int huffman_step(huffman_t *tree, int *size)
{
	int min1 = -1, min2 = -1, t, i, n = 0, slot;
	for(i = 0; i < *size; i++) {
		if(tree[i].weight >= 0) {
			n++;
			if(min2 == -1 || tree[i].weight < tree[min2].weight)
				min2 = i;
			if(min1 == -1 || tree[min2].weight < tree[min1].weight)
				t = min1, min1 = min2, min2 = t;
		}
	}
	if(n < 2) return 0;
	slot = huffman_next_free_slot(tree, size);
	tree[slot].weight = tree[min1].weight + tree[min2].weight;
	tree[slot].lch = min1;
	tree[slot].rch = min2;
	tree[min1].weight = -1;
	tree[min2].weight = -1;
	return n;
}

int huffman_initialize_tree(unsigned char *block, huffman_t *tree)
{
	int i;
	int n = 256;
	for(i = 0; i < 256; i++) {
		tree[i].weight = 0;
		tree[i].lch = tree[i].rch = -1;
	}
	for(i = 0; i < BLOCK_SIZE; i++)
		tree[block[i]].weight++;
	for(i = 0; i < 256; i++)
		if(tree[i].weight == 0)
			tree[i].weight = -2;
	while(huffman_step(tree, &n));
	return n;
}

int huffman_find_root(huffman_t *tree)
{
	int i;
	for(i = 0; ; i++)
		if(tree[i].weight >= 0)
			return i;
}

void huffman_compute_code(huffman_t *tree, int node, huffman_code_t *codes, int codelen)
{
	static huffman_code_t code;
	if(node < 0) return;
	if(tree[node].lch == -1 && tree[node].rch == -1 && node < 256) {
		code.codelen = codelen;
		memcpy(&codes[node], &code, sizeof(code));
		return;
	}
	code.code[codelen] = 0;
	huffman_compute_code(tree, tree[node].lch, codes, codelen + 1);
	code.code[codelen] = 1;
	huffman_compute_code(tree, tree[node].rch, codes, codelen + 1);
}

int huffman_deflate(huffman_t *tree, const unsigned char *input, size_t length, unsigned char *output)
{
	unsigned char ch = 0;
	int i, j, k = 0, root, outpos = 0;
	static huffman_code_t code[256];
	root = huffman_find_root(tree);
	huffman_compute_code(tree, root, code, 0);
	for(i = 0; i < length; i++) {
		for(j = 0; j < code[input[i]].codelen; j++) {
			ch = ch | ((code[input[i]].code[j] & 1) << k);
			if(++k == 8) {
				output[outpos++] = ch;
				k = 0;
				ch = 0;
			}
		}
	}
	if(k > 0)
		output[outpos++] = ch;
	return outpos;
}
void deflate(FILE *infile, FILE *outfile)
{
	static unsigned char block[BLOCK_SIZE];
	static unsigned char buffer[BLOCK_SIZE * 2];
	huffman_header_t hdr;
	static huffman_t maintree[512];
	size_t nread;
	while((nread = fread(block, 1, BLOCK_SIZE, infile)) > 0) {
		hdr.blocklen = nread;
		hdr.treesize = huffman_initialize_tree(block, maintree);
		hdr.codelen = huffman_deflate(maintree, block, nread, buffer);
		fwrite(&hdr, sizeof(hdr), 1, outfile);
		fwrite(maintree, sizeof(huffman_t), hdr.treesize, outfile);
		fwrite(buffer, 1, hdr.codelen, outfile);
	}
}
long fsize (FILE *in)
{
    long pos, length;
    pos = ftell(in);
    fseek(in, 0L, SEEK_END);
    length = ftell(in);
    fseek(in, pos, SEEK_SET);
    return length;
}
/*** LZ77 ****/
void compress(FILE *fileInput,FILE *fileOutput) {
    bool last = false;
    int inputLength = 0;
    int outputLength = 0;
    int endOffset = 0;
    int pos = -1;
    int i, size, shift, c_in;
    size_t bytesRead = (size_t) -1;
    unsigned char c;
    unsigned char array[arraySize];
    unsigned char window[windowSize];
    unsigned char buffer[bufferSize];
    unsigned char loadBuffer[bufferSize];
    unsigned char str[bufferSize];

    // Open I/O files
    //char path[30] = "input/";
    //strcat(path, inputPath)
    // If unable to open file, return alert
    //if (!fileInput) {
        //fprintf(stderr, "Unable to open fileInput %s", inputPath);
        //return 0;
    //}

    // Get fileInput length
    fseek(fileInput, 0, SEEK_END);
    inputLength = ftell(fileInput);
    fseek(fileInput, 0, SEEK_SET);

    //fprintf(stdout, "Input file size: %d bytes", inputLength);

    // If file is empty, return alert
    if (inputLength == 0)
        return 3;

    // If file length is smaller than arraySize, not worth processing
    if (inputLength < arraySize)
        return 2;

    // Load array with initial bytes
    fread(array, 1, arraySize, fileInput);

    // Write the first bytes to output file
    fwrite(array, 1, windowSize, fileOutput);

    // LZ77 logic beginning
    while (true) {
        if ((c_in = fgetc(fileInput)) == EOF)
            last = true;
        else
            c = (unsigned char) c_in;

        // Load window (dictionary)
        for (int k = 0; k < windowSize; k++)
            window[k] = array[k];

        // Load buffer (lookahead)
        for (int k = windowSize, j = 0; k < arraySize; k++, j++) {
            buffer[j] = array[k];
            str[j] = array[k];
        }

        // Search for longest match in window
        if (endOffset != 0) {
            size = bufferSize - endOffset;
            if (endOffset == bufferSize)
                break;
        }
        else {
            size = bufferSize;
        }

        pos = -1;
        for (i = size; i > 0; i--) {
            pos = findMatch(window, str, i);
            if (pos != -1)
                break;
        }

        // No match found
        // Write only one byte instead of two
        // 255 -> offset = 0, match = 0
        if (pos == -1) {
            fputc(255, fileOutput);
            fputc(buffer[0], fileOutput);
            shift = 1;
        }
        // Found match
        // offset = windowSize - position of match
        // i = number of match bytes
        // endOffset = number of bytes in lookahead buffer not to be considered (EOF)
        else {
            fputc(windowSize - pos, fileOutput);
            fputc(i, fileOutput);
            if (i == bufferSize) {
                shift = bufferSize + 1;
                if (!last)
                    fputc(c, fileOutput);
                else
                    endOffset = 1;
            }
            else {
                if (i + endOffset != bufferSize)
                    fputc(buffer[i], fileOutput);
                else
                    break;
                shift = i + 1;
            }
        }

        // Shift buffers
        for (int j = 0; j < arraySize - shift; j++)
            array[j] = array[j + shift];
        if (!last)
            array[arraySize - shift] = c;

        if (shift == 1 && last)
            endOffset++;

        // If (shift != 1) -> read more bytes from file
        if (shift != 1) {
            // Load loadBuffer with new bytes
            bytesRead = fread(loadBuffer, 1, (size_t) shift - 1, fileInput);

            // Load array with new bytes
            // Shift bytes in array, then splitted into window[] and buffer[] during next iteration
            for (int k = 0, l = arraySize - shift + 1; k < shift - 1; k++, l++)
                array[l] = loadBuffer[k];

            if (last) {
                endOffset += shift;
                continue;
            }

            if (bytesRead < shift - 1)
                endOffset = shift - 1 - bytesRead;
        }
    }

    // Get fileOutput length
    fseek(fileOutput, 0, SEEK_END);
    outputLength = ftell(fileOutput);
    fseek(fileOutput, 0, SEEK_SET);

    //fprintf(stdout, "\nOutput file size: %d bytes\n", outputLength);

    // Close I/O files
    fclose(fileInput);
    fclose(fileOutput);

    return 1;
}
int findMatch(unsigned char window[], unsigned char str[], int strLen) {
    int j, k, pos = -1;

    for (int i = 0; i <= windowSize - strLen; i++) {
        pos = k = i;

        for (j = 0; j < strLen; j++) {
            if (str[j] == window[k])
                k++;
            else
                break;
        }
        if (j == strLen)
            return pos;
    }

    return -1;
}
