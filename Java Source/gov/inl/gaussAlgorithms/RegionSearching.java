/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: RegionSearching.java
 *
 *  Description: Implements the region searching algorithm
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * contains the region search algorithm
 *
 */
public class RegionSearching {

	private final static int	RS_MIN_REGN_WIDTH = 4;	/* check with Dick... */
	private final static int	RS_MIN_PKWID = 1;	/* for region search */
	
	private RegionSearching() {
		
	}
	
	public static boolean exceedsWidth(final TreeSet<ChannelRange> regions,
			int maxRegionWidthChannels) {
		
		for (Iterator<ChannelRange> it = regions.iterator(); it.hasNext(); ) {
			ChannelRange region = it.next();
			
			if (region.widthChannels() > maxRegionWidthChannels) {
				return true;
			}
		}
		
		return false;
	}
	
	/*
	 * search	public routine to search for regions in a spectrum.
	 *
	 * A standard smoothing technique is used to generate the background
	 * curve of the entire spectrum.  Then the spectrum is compared to
	 * this background.  Wherever a significant part of the spectrum lies
	 * above the background, a region is flagged.
	 *
	 */
	public static TreeSet<ChannelRange> search(Spectrum spectrum,
			ChannelRange searchRange, WidthEquation wx,
			final TreeSet<Peak> peaks, final RegionSearchParameters parms)
	throws Exception {
	
		if (searchRange.getLastChannel() > spectrum.getLastChannel()) {
			throw new Exception("Bad search range");
		}

		if (parms.getSubtractEndsChannels() < 0) {
			throw new Exception("Bad parm for subtracting ends.");
		}
		if (parms.getMaxExpansionChannels() < 0) {
			throw new Exception("Bad parm for padding ends.");
		}
		if (parms.getThreshold() < 0) {
			throw new Exception("Bad threshold.");
		}

		// initialize
		
		int specFirstChan = spectrum.getFirstChannel();
		int numChannels = spectrum.getCounts().length;
		int maxRegionWidthChannels = parms.getMaxWidthChannels();
		double threshold = parms.getThreshold();
		RegionSearchParameters.SEARCHMODE searchMode = parms.getSearchMode();
		
		boolean[] regionFlag = new boolean[numChannels];
		int[] background = new int[numChannels];
		for (int i = 0; i < numChannels; i++) {
			regionFlag[i] = false;
			background[i] = 0;
		}

		// set up background
		
		if (RegionSearchParameters.SEARCHMODE.ALL.equals(searchMode)) {
			   initRegionBackground(wx, searchRange, spectrum, background,
					   regionFlag);
		}
		
		// force regions for existing peaks
		
		regionsForPeaks(wx, peaks, specFirstChan, numChannels, regionFlag);

		if (RegionSearchParameters.SEARCHMODE.ALL.equals(searchMode)) {
			   deleteSmallRegion(searchRange, threshold, spectrum,
					   background, maxRegionWidthChannels, regionFlag);
		}
		
		deleteSmallBackground(searchRange, specFirstChan, numChannels,
				maxRegionWidthChannels, regionFlag);

		TreeSet<ChannelRange> regions = storeRegions(searchRange,
				specFirstChan, numChannels, regionFlag);

		if (RegionSearchParameters.SEARCHMODE.ALL.equals(searchMode)) {
			regions = pruneRegions(wx, spectrum, peaks, background, threshold,
					maxRegionWidthChannels, regions);
		}

		regions = padRegions(wx, searchRange, parms.getMaxExpansionChannels(),
				parms.getSubtractEndsChannels(), maxRegionWidthChannels,
				regions);
				
		return regions;
	}

	/*
	 * deleteSmallBackground:
	 *   routine that joins regions that are close.
	 */
	private static void deleteSmallBackground(ChannelRange searchRange,
			int specFirstChan, int numChannels, int maxRegionWidthChannels,
			boolean[] regionFlag) {

		// If regions separated by one channel, join them.

		int bottomChannel = searchRange.getFirstChannel() + 6 - specFirstChan;
		bottomChannel = Math.max(1, bottomChannel);
		int topChannel = searchRange.getLastChannel() - 6 - specFirstChan;
		topChannel = Math.min((numChannels - 2), topChannel);

		int firstRegionCount = 0;
		for (int i = bottomChannel; i <= topChannel; i++) {
			if ((regionFlag[i] == false) &&
				(regionFlag[i-1] == true) && (regionFlag[i+1] == true)) {
				
				// Count second region and decide if we can join.

				int secondRegionCount = 0;
				for (int j = i + 1; j < topChannel; j++) {
					if (regionFlag[j] == true) {
						secondRegionCount++;
					} else {
						break;
					}
				}
				
				if (firstRegionCount + secondRegionCount <=
						maxRegionWidthChannels) {
					regionFlag[i] = true;
				}
			}

			if (regionFlag[i] == true) {
				firstRegionCount++;
			} else {
				firstRegionCount = 0;
			}
		}
	}

	/*
	 * deleteSmallRegion:
	 *   routine that deletes or enlarges small regions.
	 */
	private static void deleteSmallRegion(ChannelRange searchRange,
			double threshold, Spectrum spectrum, int[] background,
			int maxRegionWidthChannels, boolean[] regionFlag)
	{
		
		int specFirstChan = spectrum.getFirstChannel();
		int[] counts = spectrum.getCounts();
		int numChannels = counts.length;
		double[] sigCounts = spectrum.getSigCounts();

		int bottomChannel = searchRange.getFirstChannel() + 6 - specFirstChan;
		bottomChannel = Math.max(1, bottomChannel);
		int topChannel = searchRange.getLastChannel() - 6 - specFirstChan;
		topChannel = Math.min((numChannels - 2), topChannel);
		
		// Discard single channel regions unless both adjacent counts are
		// greater than background; and at center, y greater than
		// background + threshold.

		for (int i = bottomChannel; i <= topChannel; i++) {
			if ((regionFlag[i] == true) &&
				(regionFlag[i-1] == false) && (regionFlag[i+1] == false)) {
				if ((counts[i] >=
					 (background[i] + (int) (threshold * sigCounts[i]))) &&
					(counts[i-1] >= background[i-1]) &&
					(counts[i+1] >= background[i+1])) {
					
					// Don't create new region that is bigger than max allowed.
					int firstRegionCount = 3;
					for (int j = i - 2; j >= bottomChannel; j--) {
						if (regionFlag[j] == true) {
							firstRegionCount++;
						} else {
							break;
						}
					}
					
					int secondRegionCount = 0;
					for (int j = i + 2; j <= topChannel; j++) {
						if (regionFlag[j] == true) {
							secondRegionCount++;
						} else {
							break;
						}
					}
					
					if (firstRegionCount + secondRegionCount <=
							maxRegionWidthChannels) {
						regionFlag[i-1] = true;
						regionFlag[i+1] = true;
					} else {
						regionFlag[i] = false;
					}
				} else {
					regionFlag[i] = false;
				}
			}
		}

		// Check very first and very last channels for small regions.

		if ((regionFlag[0] == true) && (regionFlag[1] == false)) {
			regionFlag[0] = false;
		}

		if ((regionFlag[topChannel+1] == true) &&
			(regionFlag[topChannel] == false)) {
			regionFlag[topChannel+1] = false;
		}
	}

	/*
	 * initRegionBackground:
	 *   routine that does initial search using background.
	 */
	private static void initRegionBackground(WidthEquation wx,
			ChannelRange searchRange, Spectrum spectrum, int[] background,
			boolean[] regionFlag) {

		int[] counts = spectrum.getCounts();
		int numChannels = counts.length;
		double[] sigCounts = spectrum.getSigCounts();
		int specFirstChan = spectrum.getFirstChannel();
		int bottomChannel = searchRange.getFirstChannel() + 5 - specFirstChan;
		bottomChannel = Math.max(0, bottomChannel);
		int topChannel = searchRange.getLastChannel() - 5 - specFirstChan;
		topChannel = Math.min((numChannels - 1), topChannel);
		
		for (int i = 0; i < 30; i++) {
			
			// Set values in background[].
			for (int j = bottomChannel; j <= topChannel; j++) {
				double peakWidthDbl = getValidPeakwidth(wx, j);
				int peakWidthInt = (int) ((peakWidthDbl + .1) * 1.5);

				int k = j - peakWidthInt;
				k = Math.max(0, k);
				int sumTopChannel = j + peakWidthInt;
				sumTopChannel = Math.min((numChannels - 1), sumTopChannel);
				
				int sum;
				for (sum = 0; k <= sumTopChannel; k++) {
					if (regionFlag[k] == true) {
						sum += background[k];
					} else {
						sum += counts[k];
					}
	            }

	            background[j] = (sum + peakWidthInt) /
	            		        ((2 * peakWidthInt) + 1);
			}

			// Set values in regionFlag[].

			boolean change = false;
			for (int j = bottomChannel; j <= topChannel; j++) {
				if ((regionFlag[j] == false) &&
	                ((background[j] + (int) (2 * sigCounts[j])) <=
	                	counts[j]) &&
	                (counts[j] > 1)) {
					regionFlag[j] = true;
					change = true;
				}
			}

			if (change == false) {
				break;
			}
		} // end i loop
	}

	/*
	 * getValidPeakwidth:
	 *   routine to calculate a valid peakwidth at the indicated channel.
	 */
	private static double getValidPeakwidth(WidthEquation wx, double channel) {
				
		double peakwidth = RS_MIN_PKWID;
		
		try {
			peakwidth = wx.getPeakwidth(channel);
			peakwidth = Math.max(RS_MIN_PKWID, peakwidth);
		} catch (Exception e) {
			// leave as default
		}

		return peakwidth;
	}

	/*
	 * pruneRegions:
	 *   routine that deletes regions that are not needed for an
	 *   existing peak and are not wide enough above background.
	 */
	private static TreeSet<ChannelRange> pruneRegions(WidthEquation wx,
			Spectrum spectrum, final TreeSet<Peak> peaks, int[] background,
			double threshold, int maxRegionWidthChannels,
			final TreeSet<ChannelRange> regions) {

		TreeSet<ChannelRange> newRegionList1 = new TreeSet<ChannelRange>();		
		int numRegions = regions.size();
		if (0 >= numRegions) {
			return newRegionList1;
		}

		// If region contains peak from peaklist (search peaks), keep it.
		// Else if region has < peakwidth points above background near
		// the highest point in the region, then discard the region.

		int specFirstChan = spectrum.getFirstChannel();
		int[] counts = spectrum.getCounts();
		double[] sigCounts = spectrum.getSigCounts();
		
		for (Iterator<ChannelRange> it = regions.iterator(); it.hasNext(); ) {
			
			boolean regionWithPeak = false;
			ChannelRange region = it.next();
			
			for (Iterator<Peak> itp1 = peaks.iterator(); itp1.hasNext(); ) {
				Peak peak = itp1.next();
				
				if (peak.inChannelRange(region)) {
					regionWithPeak = true;
					break;
				}				
			}

			if (regionWithPeak) {
				newRegionList1.add(region);
			} else {
				int maxDiff = 0;
				int bottomChannel = region.getFirstChannel() - specFirstChan;
				int topChannel = region.getLastChannel() - specFirstChan;
				int tempPeak = bottomChannel;
				for (int j = bottomChannel; j <= topChannel; j++) {
					int diff = counts[j] - background[j];
					if (diff > maxDiff) {
						maxDiff = diff;
						tempPeak = j;
					}
				}
	      
				double peakWidthDbl = getValidPeakwidth(wx, tempPeak);
				int peakWidthInt = (int) (((peakWidthDbl + .5) / 2.0) - 1.0);
				peakWidthInt = Math.max(peakWidthInt, RS_MIN_PKWID);

				int belowBackgroundCount = 0;
				bottomChannel = tempPeak - peakWidthInt - specFirstChan;
				topChannel = tempPeak + peakWidthInt + 1 - specFirstChan;
				for (int j = bottomChannel; j <= topChannel; j++) {
					if (counts[j] <= background[j]) {
						belowBackgroundCount++;
					}
				}
				
				if ((belowBackgroundCount <= 1) &&
					(counts[tempPeak] > background[tempPeak] + (int)
					                    (threshold * sigCounts[tempPeak]))) {
					newRegionList1.add(region);
				}
			} // no peaks in region
		} // loop over regions
		
		// If regions close, data between them is above background,
		// and combined regions have <= 4 peaks, then join regions.
		// Don't join regions if it would exceed max size allowed.
				
		Iterator<ChannelRange> it = newRegionList1.iterator();
		if (!it.hasNext()) {
			return newRegionList1;
		}
		ChannelRange firstRegion = it.next();		

		TreeSet<ChannelRange> newRegionList2 = new TreeSet<ChannelRange>();

		while (it.hasNext()) {
			ChannelRange secondRegion = it.next();
			boolean joinedRegions = false;

			// decide whether to join regions
						
			// Through Gauss Algorithms 1.30 (1/16/2012), a peakwidth
			// calculated at channel 3000 was used inside the loop.
			// From version 1.31 through 2.1 (1/17/2012 - 3/11/2013),
			// a peakwidth at a very small channel was incorrectly used.
			// Fixed in version 2.2 (4/2013) to use peakwidth at midpoint
			// between regions.
			
			// calculate peakwidth at midpoint between regions
			double channel = ((double) firstRegion.getLastChannel() +
					secondRegion.getFirstChannel()) / 2.0;
			double peakWidthDbl = getValidPeakwidth(wx, channel);
			int peakWidthInt = (int) (peakWidthDbl + .5);
			
			if ((secondRegion.getFirstChannel() -
					firstRegion.getLastChannel()) <= peakWidthInt) {
				int bottomChannel = firstRegion.getLastChannel() + 1 -
						specFirstChan;
				int topChannel = secondRegion.getFirstChannel() -
						specFirstChan;
				
				boolean belowBackground = false;
				for (int j = bottomChannel; j < topChannel; j++) {
					if (counts[j] < background[j]) {
						belowBackground = true;
						break;
					}
	            }
				
				if (!belowBackground) {
					// count the peaks that lie in the proposed joined region
					int peakRegionCount = 0;
					for (Iterator<Peak> itp2 = peaks.iterator(); itp2.hasNext(); ) {
						Peak peak = itp2.next();
						
						if (peak.isChannelValid() == true) {
							double peakChannel = peak.getChannel();
							if ((peakChannel >=
									firstRegion.getFirstChannel()) &&
								(peakChannel <=
									secondRegion.getLastChannel())) {
								peakRegionCount++;
							}
						}
					}

					if ((peakRegionCount <= 4) &&
						(secondRegion.getLastChannel() -
								secondRegion.getFirstChannel() <= 90) &&
						(secondRegion.getLastChannel() -
								firstRegion.getFirstChannel() <=
									maxRegionWidthChannels)) {

						// Join regions.
						ChannelRange newRegion = new ChannelRange(
								firstRegion.getFirstChannel(),
								secondRegion.getLastChannel());
						joinedRegions = true;
						
						// keep new region around for next attempt to join
						secondRegion = newRegion;
					}
				}
			}
			
			if (!joinedRegions) {
				newRegionList2.add(firstRegion);
			}
			firstRegion = secondRegion;			
			
		}
		// now add whatever is left in firstRegion to the list2
		newRegionList2.add(firstRegion);		
		
		return newRegionList2;
	}

	/*
	 * padRegions:
	 *   routine that pads the ends of regions using the irw & irch
	 *   parameters.
	 */

	private static TreeSet<ChannelRange> padRegions(WidthEquation wx,
			ChannelRange searchRange, int maxExpansionChannels,
			int subtractEndsChannels, int maxRegionWidthChannels,
			final TreeSet<ChannelRange> regions) {
		
		TreeSet<ChannelRange> newRegions = new TreeSet<ChannelRange>();
		
		Iterator<ChannelRange> it = regions.iterator();
		if (!it.hasNext()) {
			return newRegions;
		}
		
		// Pad lower end of first region.
		
		ChannelRange firstRegion = it.next();
		int firstChan = firstRegion.getFirstChannel();
		int lastChan = firstRegion.getLastChannel();
		double peakWidthDbl = getValidPeakwidth(wx, firstChan);
		int peakWidthInt = (int) (peakWidthDbl + .5);

		if ((lastChan - firstChan + (2 * peakWidthInt) <=
				maxRegionWidthChannels) &&
			(firstChan >= searchRange.getFirstChannel())) {
			int newFirstChannel =
			        Math.max(searchRange.getFirstChannel(),
			        		firstChan - (2 * peakWidthInt));
			firstRegion = new ChannelRange(newFirstChannel, lastChan);
		}
		
		// Loop through regions starting with second.
		// Pad upper end of prev and lower end of curr regions.
		
		while (it.hasNext()) {
			ChannelRange secondRegion = it.next();
			
			peakWidthDbl = getValidPeakwidth(wx,
					secondRegion.getFirstChannel());
			peakWidthInt = (int) (peakWidthDbl + .5);

			int regionGap = secondRegion.getFirstChannel() -
					firstRegion.getLastChannel();
			int regionPad = regionGap;
			
			// In Gauss VII, subtractEndsChannels was called "irch", which is
			// the number of channels to subtract from ends of regions
			// by removing channels from the padding.

			if (regionGap >= subtractEndsChannels) {
		         regionPad = regionGap - subtractEndsChannels;
			}
		
			// In Gauss VII, maxExpansionChannels was called "irw", and it
			// controls region expansion in multiples of peakwidth
			// if gap is big enough.

			if ((regionGap / peakWidthInt) > maxExpansionChannels) {
		         regionPad = maxExpansionChannels * peakWidthInt;
			}
			
			int firstRegionWidth = firstRegion.getLastChannel() -
					firstRegion.getFirstChannel();

			int firstRegionPad = regionPad;
			if ((firstRegionWidth + regionPad) > maxRegionWidthChannels) {
				firstRegionPad = maxRegionWidthChannels - firstRegionWidth - 1;
				firstRegionPad = Math.max(0, firstRegionPad);
			}
			
			firstRegion = new ChannelRange(
					firstRegion.getFirstChannel(),
					firstRegion.getLastChannel() + firstRegionPad);
			
			int secondRegionWidth = secondRegion.getLastChannel() -
					secondRegion.getFirstChannel();
			
			int secondRegionPad = regionPad;
			if ((secondRegionWidth + regionPad) > maxRegionWidthChannels) {
				secondRegionPad = maxRegionWidthChannels -
						secondRegionWidth - 1;
				secondRegionPad = Math.max(0, secondRegionPad);
			}
			
			secondRegion = new ChannelRange(
					secondRegion.getFirstChannel() - secondRegionPad,
					secondRegion.getLastChannel());
			
			newRegions.add(firstRegion);
			firstRegion = secondRegion;
		}
				
		// Add pad to upper end of last region. (stored in "firstRegion")

		peakWidthDbl = getValidPeakwidth(wx, firstRegion.getLastChannel());
		peakWidthInt = (int) (peakWidthDbl + .5);
		
		int regionGap = searchRange.getLastChannel() - 5 -
				firstRegion.getLastChannel();
		int regionPad = Math.max(0, regionGap - subtractEndsChannels);
		
		if (regionGap / peakWidthInt > maxExpansionChannels) {
			regionPad = maxExpansionChannels * peakWidthInt;
		}

		int firstRegionWidth = firstRegion.getLastChannel() -
				firstRegion.getFirstChannel();
		
		if (firstRegionWidth + regionPad >= RS_MIN_REGN_WIDTH) {
			if (firstRegionWidth + regionPad > maxRegionWidthChannels) {
				regionPad = maxRegionWidthChannels - firstRegionWidth - 1;
				regionPad = Math.max(0, regionPad);
			}
			
			firstRegion = new ChannelRange(
					firstRegion.getFirstChannel(),
					firstRegion.getLastChannel() + regionPad);
			newRegions.add(firstRegion);
		}

		return newRegions;
	}

	/*
	 * regionsForPeaks:
	 *   routine that forces regions for existing peaks.
	 */
	private static void regionsForPeaks(WidthEquation wx,
			final TreeSet<Peak> peaks, int specFirstChan, int numChannels,
			boolean[] regionFlag) {

		// Assure search peaks are included.
		
		for (Iterator<Peak> it = peaks.iterator(); it.hasNext(); ) {
			Peak peak = it.next();
			
			if (peak.isChannelValid()) {
				double channel = peak.getChannel();
				
				double peakWidthDbl = getValidPeakwidth(wx, channel);
				
				// Set regionFlag TRUE in area around peak.
				
				int bottomChannel = (int) (channel - specFirstChan -
						                   peakWidthDbl);
				bottomChannel = Math.max(0, bottomChannel);
				int topChannel = (int) (channel - specFirstChan +
						                peakWidthDbl + .5);
				topChannel = Math.min(numChannels - 1, topChannel);

				for (int j = bottomChannel; j <= topChannel; j++) {
					regionFlag[j] = true;
				}
			}
		}
	}

	/*
	 * storeRegions:
	 *   routine that translates an array of flags into a list of regions.
	 */
	private static TreeSet<ChannelRange> storeRegions(ChannelRange searchRange,
			int specFirstChan, int numChannels, boolean[] regionFlag) {

	/*
	 * Comments and change by Egger on 9/10/99.
	 *
	 * In original Gauss VII fortran, the scan for regions to store ranges
	 * from first channel in search range + 5
	 * to last channel in search range - 5.
	 *
	 * This potentially misses a region that was flagged for an
	 * existing peak by the regionsForPeaks() routine because this
	 * region could start before the scanning starts.
	 *
	 * So start the scanning at the beginning of the search range.
	 *
	 * Egger change on 9/17/2001 - first region could start in first channel.
	 */
		TreeSet<ChannelRange> regions = new TreeSet<ChannelRange>();

		// Record start and end of each region.

		int bottomChannel = searchRange.getFirstChannel() - specFirstChan;
		bottomChannel = Math.max(0, bottomChannel);
		int topChannel = searchRange.getLastChannel() - specFirstChan;
		topChannel = Math.min((numChannels - 1), topChannel);

		boolean withinRegion = false;
		int newRegionStart = 0;
		
		for (int i = bottomChannel; i <= topChannel; i++) {
			if ((withinRegion == false) && (regionFlag[i] == true)) {
				withinRegion = true;
				newRegionStart = i + specFirstChan;
			} else if ((withinRegion == true) && (regionFlag[i] == false)) {
				int newRegionEnd = i - 1 + specFirstChan;
				ChannelRange newRegion =
						new ChannelRange(newRegionStart, newRegionEnd);
				regions.add(newRegion);
				withinRegion = false;
				newRegionStart = 0;
			}
		}

		// If the last region did not end before the search range,
		// then add one last region that ends with the search range.

		if (withinRegion == true) {
			int newRegionEnd = searchRange.getLastChannel();
			ChannelRange newRegion =
					new ChannelRange(newRegionStart, newRegionEnd);
			regions.add(newRegion);
		}
		
		return regions;
	}
}
