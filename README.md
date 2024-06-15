\documentclass[12pt]{article}

\usepackage{fullpage}
\usepackage{graphicx}
\usepackage{subcaption}
\usepackage{amssymb}
\usepackage{fancyvrb}
\usepackage{bm}
\usepackage{float}
\usepackage{amsmath} 
\usepackage[export]{adjustbox}
\setlength{\parindent}{0pt}
\usepackage{listings}
\usepackage{xcolor}
\usepackage{placeins}


\lstset{
    language=Python, % Choose the programming language (replace with your language)
    basicstyle=\ttfamily\footnotesize,
    keywordstyle=\color{blue},
    stringstyle=\color{red},
    commentstyle=\color{green},
    breaklines=true,
    frame=lines
}

\title{\LARGE\textbf{CSC3050 Assignment3 Report}}
\author{\normalsize\textbf{Songlin Zhao, 120090346}}

\date{\small{May 10th, 2024}}
\begin{document}
\maketitle

\section{Usage}
Before testing, please put the folder ./cache-trance and ./riscv-elf inside the working directory, with the same level as ./src and ./include
\subsection{Run Single-Level Cache Simulation}
\begin{verbatim}
cd /the_working_space
chmod +x build1.sh
./build1.sh
# Now the single-level cache simulator will be runned, 
# and the .csv file ./src/analysis_p1.csv will be generated 
\end{verbatim}

\subsection{Run Multi-Level Cache Simulation}
\begin{verbatim}
chmod +x build2.sh
./build2.sh
# Now the multi-level cache simulator will be runned, 
# and the .csv file ./src/analysis_p2.csv will be generated 
\end{verbatim}

\subsection{Run Integration with CPU Simulator}
\begin{verbatim}
chmod +x build3.sh
./build3.sh
./build/Simulator riscv-elf/quicksort.riscv  # run without cache
./build/Simulator -c riscv-elf/quicksort.riscv  # run with cache
# the statistics will be printed in the terminal
\end{verbatim}

\section{Single-Level Cache Simulation}
In the first part of this project, I implemented a single-level cache. The cache takes parameters including underlying memory, hit latency, cache size, block size, associativity, write back, write allocate, .etc.
\subsection{Implementation Details}
A cache consists of $cacheSize / blockSize$ blocks, and each block contains the following parameters. The valid bit, dirty bit, tag, set number are defined by the cache. The lastAccess parameter records the last access cycle of the current block, and data is an array of uint\_8, which is essentially an array of bytes. 
\begin{lstlisting}
struct Block {
    bool valid;   // valid bit
    bool dirty;   // dirty bit
    uint32_t tag; // tag
    uint32_t setNum;
    uint32_t lastAccess;
    std::vector<uint8_t> data; // data in each block, an array of uint_8
};
\end{lstlisting}

Each Cache contains $cacheSize/blockSize$ blocks, which is represented by an array of blocks (\verb|std::vector<Block> blocks|). Two essential functions of the Cache class is \verb|get_byte()| and \verb|set_byte()|.

\subsubsection{get\_byte}
The \verb|uint8_t get_byte(uint32_t addr)| function takes the address of the memory as the parameter, and return a 8-bit unsigned integer. The overall logic of this function is as follows:
\begin{enumerate}
    \item Given address, get the tag number, set number, and the offset.
    \item Check whether the block of this address is already in cache, i.e., the valid bit is 1, and the set number matches. If so, just return the byte in cache.
    \item If not in cache, this function tries to find the addr in memory, and bring the whole block from memory to cache. If there is no enough space for the new block, the cache will evict a block from the cache using LRU algorithm. Finally, return the byte which is newly put in cache.
\end{enumerate}

\subsubsection{set\_byte}
The \verb|void set_byte(uint32_t addr)| function takes the address of the memory as the parameter. The overall logic of this function is as follows (assuming write back and write allocate policy is used):
\begin{enumerate}
    \item Given address, get the tag number, set number, and the offset.
    \item Check whether the block of this address is already in cache, i.e., the valid bit is 1, and the set number matches. If so, write the corresponding byte in cache, and set dirty bit to 1.
    \item If not in cache, this function tries to find the addr in memory, and bring the whole block from memory to cache. If there is no enough space for the new block, the cache will evict a block from the cache using LRU algorithm. Finally, write the corresponding byte in cache, and set dirty bit to 1.
\end{enumerate}

\subsubsection{Write Back and Write Allocate}
There are four cases for write back and write allocate policies:
\begin{enumerate}
    \item Write-Back = True, Write-Allocate = True: When a write miss occurs, the cache block containing the target address is loaded into the cache, and then the write operation is performed on the cached block. The modified block is marked as dirty but is not immediately written back to main memory. The dirty data is only written back to the main memory when the cache block is evicted.
    \item Write-Back = True, Write-Allocate = False: When a write operation occurs, the data is written directly to main memory. However, if the data is already in the cache, it is updated in the cache and marked as dirty. It will only be written back to the main memory upon eviction.
    \item Write-Back = False, Write-Allocate = True: When a write miss occurs, the cache block is loaded into the cache, and the write operation is executed on the cache. However, all writes to the cache will also be written to main memory.
    \item Write-Back = False, Write-Allocate = False: When a write operation occurs, the data is written directly to main memory. No write back is used.
\end{enumerate}


\subsection{Performance analysis}
The following image shows how different block size affects cache performance. We can observe that when holding all other conditions, the larger the block size, the better the cache performance. This could be explained by the fact that larger block size can bring a larger block of data during a cache miss, which increases the possibility of the next hit.
\begin{figure}[htpb]
  \centering
  % First image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{block1.png}
    \caption{Block Size vs. CPI}
    \label{fig:transformer1}
  \end{minipage}
  \hspace*{\stretch{0.000005}}
  % Second image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{block2.png}
    \caption{Block Size vs. Miss Rate}
    \label{fig:transformer2}
  \end{minipage}
  \caption{Effects of block size on performance}
\end{figure}

\FloatBarrier

The following image shows how different cache size affects cache performance. We can observe that when holding all other conditions, when we increase the cache size, the performance improves significantly. However, under the same cache size, the performance of different block sizes does vary that much. The larger cache size means more faster memories, which will definitely increase the performance.
\begin{figure}[htpb]
  \centering
  % First image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{cache1.png}
    \caption{Cache Size vs. CPI}
    \label{fig:transformer1}
  \end{minipage}
  \hspace*{\stretch{0.000005}}
  % Second image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{cache1.png}
    \caption{Cache Size vs. Miss Rate}
    \label{fig:transformer2}
  \end{minipage}
  \caption{Effects of cache size on performance}
\end{figure}

\FloatBarrier

The following image demonstrates the influence of associativity on cache performance. When the cache size is 4KB, increase the associativity can cause better performance at first, but worse performance latter. However, when the cache size is 16KB, the relationship is positive, while when the cache size is 64KB, the relationship is negative. This shows that increasing associativity does not necessarily mean increasing or decreasing the performance.
\begin{figure}[htpb]
  \centering
  % First image
  \begin{minipage}{0.32\linewidth}
    \includegraphics[width=\linewidth]{asso4.png}
    \caption{Cache Size 4KB}
    \label{fig:transformer1}
  \end{minipage}
  \hspace*{\stretch{0.000005}}
  % Second image
  \begin{minipage}{0.32\linewidth}
    \includegraphics[width=\linewidth]{asso16.png}
    \caption{Cache Size 16KB}
    \label{fig:transformer2}
  \end{minipage}
  \caption{Effects of cache size on performance}
  \hspace*{\stretch{0.000005}}
  \begin{minipage}{0.32\linewidth}
    \includegraphics[width=\linewidth]{asso64.png}
    \caption{Cache Size 64KB}
    \label{fig:transformer3}
  \end{minipage}
  \caption{Effects of cache size on performance}
\end{figure}

\section{Multi-Level Cache Simulation}

\subsection{Implementation Details}

\subsubsection{Inclusive Cache}
The \verb|get_byte()| and \verb|set_byte()|  function is modified to enable multi-level inclusive cache. In inclusive cache, if a block of data is present at a certain level, all lower levels will contains that block of data.
\begin{enumerate}
    \item When there is a cache miss, this function recursively get the byte from lower caches and place them in the current level until the data is present, which maintains inclusivity.
    \item When there is a eviction occurs, we need to write the data to lower level. We only write to the next lower level, not all lower levels. 
    \item I implemented the back invalidation when the data is evicted from a lower level, all data from higher levels will also be evicted. However, by using LRU and recording the lastAccess variable of all blocks, there will be no case when the data is evicted from a lower level where higher level caches contains that block of data.
    \item The dirty bit in multi-level cache means the block in that level is different from lower level cache, so during writing to an address, the dirty bit is only set to the level 1 cache. All lower caches is not dirty. 
\end{enumerate}

\subsubsection{exclusive Cache}
For exclusive cache, a block of data only exists in a certain level of cache. To pertain this property, the following eviction and replacement policy is used.
\begin{enumerate}
    \item When there is a cache miss, the function finds the exact level of cache that contains this block of data, and the data is directly returned to the L1 cache.
    \item When there is a eviction occurs, we need to write the data to lower level. We only write to the next lower level, not all lower levels. 
    \item When a block is brought up from a lower level cache or evicted to a lower level cache, the dirty bit is preserved.
\end{enumerate}

\subsubsection{Victim Cache}
The victim cache is only connected to the L1 cache, and stores the recently evicted cache from the L1 cache. The vitim cache is only 8 blocks and it is fully associative.
\begin{enumerate}
    \item When a data access results in a miss in the L1 cache, the program checks the victim cache to see if the requested data is available there. If the data is in victim cache, just return it. If the data is not found in the victim cache either, he data is then fetched from the L2 cache.
    \item The victim cache uses LRU policy to handle which block to evict.
\end{enumerate}

\subsection{Performance analysis}
The following figure shows the performance of single-level cache, inclusive three-level cache without victim cache, exclusive three-level cache without victim cache, and inclusive three-level cache with victim cache. 
\begin{figure}[H]
  \centering
  \includegraphics[width=0.6\linewidth]{p2.png}
\caption{The Cache performance of different settings}
\label{fig:structure}
\end{figure}

\begin{figure}[H]
  \centering
  \includegraphics[width=0.6\linewidth]{default.png}
\caption{Default cache setting used in multi-level cache simulation}
\label{fig:structure}
\end{figure}
\subsubsection{Single and Multi}
The performance of multi-level cache is 3 times faster than single-level cache. This is because the addition of L2 and L3 cache shortens the distance between cpu and memory. \\
\subsubsection{Inclusive and Exclusive}
The performance of inclusive and exclusive cache is similar, while exclusive cache might be a little bit slower. Although exlusive cache saves more space for lower-level data, it can be a little bit slower. This can be explained by the fact that the L1 cache is small. \textbf{For inclusive cache, the block is only evicted from L1 cache when the L1 cache is full, and the dirty bit is 1. However, for exclusive cache, the block is evicted only if the L1 cache is full, no matter whether it is dirty}. As a result, it causes more cycles during testing because exclusive cache takes more time to write to L2 caches. The other latencies for inclusive and exclusive caches are the same. They both checks all absent caches during cache miss. \textbf{During cycles calculation, if a cache miss occurs, inclusive cache might both look up and write to the cache, exclusive cache might only look up the cache without writing to it, however, I only calculated once for the latency to mimic the real-world behaviour.} \\ 
\subsubsection{With and Without victim}
The addition of victim cache increases the total cycles a little bit, because the size of the victim cache is only 8 blocks, and accessing it causes extra time. \textbf{The access latency of the victim cache is set to 2}. However, during each cache miss, 2 extra cycles will be added, but the chances to find a block in victim cache is really low, so the addition of victim cache would increase the total cycles. \textbf{If I set the hit latency of victim cache to be 0, the caches with victim indeed works a little bit faster, which conforms my expectation.}


\section{Integration with CPU Simulator}

\subsection{Implementation Details}
The time to access memory is set to be 100. The settings of cache is set to be in Table 4. In this part, I used the MemoryManager.cpp, BranchPredictor.cpp, Simulator.cpp, and MainCPU.cpp from the reference. I used my own Cache.cpp, and a little modification to the MainCPU.cpp and Simulator.cpp.
\subsection{Performance analysis}
\begin{figure}[htpb]
  \centering
  % First image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{q.png}
    \caption{Without Cache}
    \label{fig:transformer1}
  \end{minipage}
  \hspace*{\stretch{0.000005}}
  % Second image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{qc.png}
    \caption{With Cache}
    % \label{fig:transformer2}
  \end{minipage}
  \caption{quicksort.riscv}
\end{figure}

\begin{figure}[htpb]
  \centering
  % First image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{a.png}
    \caption{Without Cache}
    \label{fig:transformer1}
  \end{minipage}
  \hspace*{\stretch{0.000005}}
  % Second image
  \begin{minipage}{0.48\linewidth}
    \includegraphics[width=\linewidth]{ac.png}
    \caption{With Cache}
    % \label{fig:transformer2}
  \end{minipage}
  \caption{ackermann.riscv}
\end{figure}

For quicksort.riscv, the CPI is 49.2 without cache, but it is 2.07 with cache. For ackermann.riscv, the CPI is 39.3 without cache, but it is 1.77 with cache. The Cache significantly improves the performance of programs.




\end{document}