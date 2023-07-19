/*
* Copyright 2023 Wargaming.net Limited
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/ 
#pragma once

// Engine headers

namespace VSPGitHelpers
{
	struct FDivergence
	{
		int32 Ahead = 0;
		int32 Behind = 0;
	};

	/// Git version and capabilites extracted from the string "git version 2.11.0.windows.3"
	struct FGitVersion
	{
		// Git version extracted from the string "git version 2.11.0.windows.3" (Windows) or "git version 2.11.0" (Linux/Mac/Cygwin/WSL)
		int Major; // 2	Major version number
		int Minor; // 11	Minor version number
		int Patch; // 0	Patch/bugfix number
		int Windows; // 3	Windows specific revision number (under Windows only)

		uint32 bHasGitLfs : 1;
		uint32 bHasGitLfsLocking : 1;

		FGitVersion()
			: Major( 0 )
			, Minor( 0 )
			, Patch( 0 )
			, Windows( 0 )
			, bHasGitLfs( false )
			, bHasGitLfsLocking( false )
		{}

		bool IsGreaterOrEqualThan( int InMajor, int InMinor ) const
		{
			return ( Major > InMajor ) || ( Major == InMajor && ( Minor >= InMinor ) );
		}
	};

	/**
	* Parse information on a file locked with Git LFS
	*/
	class FGitLfsLockInfo
	{
	public:
		FGitLfsLockInfo( const FString& InStatus );

		FString LocalFilename; ///< Filename on disk
		FString LockUser; ///< Name of user who has file locked
		int32 Id; ///< ID lock
	};

	/**
	* Extract the SHA1 identifier and size of a blob (file) from a Git "ls-tree" command.
	*
	* Example output for the command git ls-tree --long 7fdaeb2 Content/Blueprints/BP_Test.uasset
	100644 blob a14347dc3b589b78fb19ba62a7e3982f343718bc   70731	Content/Blueprints/BP_Test.uasset
	*/
	class FGitLsTreeParser
	{
	public:
		/** Parse the unmerge status: extract the base SHA1 identifier of the file */
		FGitLsTreeParser( const TArray< FText >& InResults )
		{
			const FString& FirstResult = InResults[ 0 ].ToString();
			FileHash = FirstResult.Mid( 12, 40 );
			int32 IdxTab;
			if ( FirstResult.FindChar( '\t', IdxTab ) )
			{
				const FString SizeString = FirstResult.Mid( 53, IdxTab - 53 );
				FileSize = FCString::Atoi( *SizeString );
			}
		}

		FString FileHash; ///< SHA1 Id of the file (warning: not the commit Id)
		int32 FileSize; ///< Size of the file (in bytes)
	};

	/**
	* Helper struct for maintaining temporary files for passing to commands
	*/
	class FVSPGitScopedTempFile
	{
	public:
		/** Constructor - open & write string to temp file */
		FVSPGitScopedTempFile( const FText& InText );
		/** Destructor - delete temp file */
		~FVSPGitScopedTempFile();
		/** Get the filename of this temp file - empty if it failed to be created */
		const FString& GetFilename() const;
	private:
		/** The filename we are writing to */
		FString Filename;
	};

	/** Match the relative filename of a Git status result with a provided absolute filename */
	class FVSPGitStatusFileMatcher
	{
	public:
		FVSPGitStatusFileMatcher( const FString& InAbsoluteFilename )
			: AbsoluteFilename( InAbsoluteFilename )
		{}

		bool operator()( const FString& InResult ) const;

	private:
		const FString& AbsoluteFilename;
	};

	void ParseGitVersion( const FString& InVersionString, FGitVersion* OutGitVersion );

	void FindGitCapabilities( FGitVersion* OutGitVersion );

	void FindGitLfsCapabilities( FGitVersion* OutGitVersion );

	void ParseCumulativeStream( FString& CurrentCumulativeString, TQueue< FString >& OutQueueLines );

	bool GetLockableRuleList( const FString& InRepoRoot, TArray< FString >& OutRules );

	bool IsFileLockable( const FString& InFileName, const TArray< FString >& InLockableRules );

	bool IsLockByMe( const FString& InLockUser );

	void GetMissingVsExistingFiles(
		const TArray< FString >& InFiles,
		TArray< FString >& OutMissingFiles,
		TArray< FString >& OutAllExistingFiles,
		TArray< FString >& OutOtherThanAddedExistingFiles );

	FText ParseCommitResults( const TArray< FText >& InResults );

	TArray< FString > AbsoluteAndFilteringFilenames( const TArray< FString >& InFileNames, const FString& InRelativeTo );

	TArray< FString > CheckAndMakeRelativeFilenames( const TArray< FString >& InFileNames, const FString& InRelativeTo );

	FString RelativeByRepoPathToFull( const FString& InRelativePath );

	TArray< FString > RelativeByRepoPathToFull( const TArray< FString >& InRelativePaths );
}
