/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: PeakSearching.java
 *
 *  Description: Implements the peak searching algorithm
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * contains the peak search algorithm
 *
 */
public class PeakSearching {
	
	private final static int PS_UPDATE_INTERVAL = 10;
	private final static int PS_MIN_PEAKWIDTH = 1;
	private final static int PS_MAX_PEAKWIDTH = 10;
	private final static int PS_MIN_SQWAV = (PS_MIN_PEAKWIDTH * 3);	/* for peak search */
	private final static int PS_MAX_FITWIDTH_ODD = 1001; /* for refining location of peak */


	// constructor
	private PeakSearching() {
		// singleton. only provides static methods.
	}
	
	// public methods

	public static TreeSet<Peak> pruneRqdPks(WidthEquation wx,
			final TreeSet<Peak> searchPks, final TreeSet<Peak> currRqdPks) {

		TreeSet<Peak> newRqdPks = new TreeSet<Peak>();

		for (Iterator<Peak> itrp = currRqdPks.iterator(); itrp.hasNext(); ) {
			Peak rqdPeak = itrp.next();

			if (rqdPeak.isChannelValid()) {
				boolean save = true;

				for (Iterator<Peak> itsp = searchPks.iterator();
					 itsp.hasNext(); ) {
					Peak srchPeak = itsp.next();

					if (srchPeak.isChannelValid()) {
						double peakWidth;
						try {
							peakWidth = wx.getPeakwidth(srchPeak.getChannel());
						} catch (Exception e) {
							peakWidth = 0;
						}
						if (peakWidth <= 0.0) {
							peakWidth = 3;
						}
						double threshold = .2 * peakWidth;

						if (Math.abs(rqdPeak.getChannel() -
								srchPeak.getChannel()) < threshold) {
							save = false;
							break;
						}
					}
				}

				if (save) {
					newRqdPks.add(rqdPeak);
				}
			}
		}

		return newRqdPks;
	}
	
	/*
	 * search - public routine to search for peaks in a channel range
	 *          of a spectrum. Returns a list of channel locations
	 *          representing peak locations.
	 *
	 *
	 * First, construct a square wave with zero area, and width calculated
	 * using the width equation for each 1K range of the spectrum.  The wave
	 * looks approximately like:
	 *
	 *                 -----
	 *                 |   |
	 *                 |   |
	 *     ***************************** <-- y=0
	 *             |   |   |   |
	 *             -----   -----
	 *
	 * Then take the cross product of the square wave curve with the uncertainties
	 * of the spectrum counts, multiplying the square wave to the uncertainties,
	 * a waves' width at a time.  Save the result of each multiplication.
	 * (The uncertainties of the counts should be previously calculated with
	 *  the subroutine "GL_set_sigcount" in utilities.c. The uncertainties
	 *  are approximately the square root of the counts.)
	 *
	 * This process effectively reinforces the peak shapes and washes out
	 * the noise and background.
	 *
	 * When the resulting shape is examined, and a local maximum is greater
	 * than the threshold passed into the search function, the location is
	 * tagged as a peak.
	 *
	 * Taking the cross product of the square wave with the counts is discussed in:
	 *   K. Debertin & R. G. Helmer, Gamma- and X-Ray Spectrometry with
	 *   Semiconductor Detectors, (Amsterdam: Elsevier Science B.V., 1988),
	 *   p. 172-175. 
	 *
	 * Using the count uncertainties in the cross product is discussed in:
	 *   C. M. McCullagh & R. G. Helmer, GAUSS VII A Computer Program for the
	 *   Analysis of Gamma-Ray Spectra from Ge Semiconductor Spectrometers,
	 *   (EGG-PHYS-5890, October 1982), p. 5-7.
	 * This reference states that the count uncertainties are used as an
	 * attempt to make the sensitivity more independent of the magnitude
	 * of the counts.
	 *
	 */
	/*
	 * threshold controls the pruning out of insignificant peaks. Large
	 * values increase the pruning.
	 *
	 *			for low search sensitivity, use  threshold = 20
	 *			for med search sensitivity, use  threshold = 10
	 *			for high search sensitivity, use threshold =  5
	 *
	 */
	public static PeakSearchResults search(Spectrum spectrum,
			ChannelRange searchRange, WidthEquation wx, int threshold)
		throws Exception {
		
		int firstSearchChannel = searchRange.getFirstChannel();
		int lastSearchChannel = searchRange.getLastChannel();
		
		if ((lastSearchChannel > spectrum.getLastChannel()) ||
			(firstSearchChannel > lastSearchChannel)) {
			throw new Exception("bad channel range");
		}
		if (threshold <= 0) {
			throw new Exception("bad threshold");
		}
		
		// calculate hiChannel to leave room for cross product
		double hiChannel = lastSearchChannel -
				(int) (3 * getSquareWaveWidth(wx, lastSearchChannel));
				
		int numChannels = spectrum.getCounts().length;
		int[] crossProducts = new int[numChannels];
		for (int i = 0; i < numChannels; i++) {
			crossProducts[i] = 0;
		}

		// initialize sqwav_wid
		int squareWaveWidth = getSquareWaveWidth(wx, firstSearchChannel);

		// Determine first multiple of update interval after the start of the channels
		// to be searched over.
		
		// increment "i" until the interval multiples exceed the starting point
		int i = 1;
		while (firstSearchChannel >= (PS_UPDATE_INTERVAL * i)) {
			i++;
		}
		// now initialize the multiple of interval
		int multInterval = PS_UPDATE_INTERVAL * i;
		
		// Make integer array of spectrum uncertainties for efficient
		// use inside the cross product loop.
		double[] uncertainties = spectrum.getSigCounts();
		int numUncertainties = uncertainties.length;
		int[] uncertaintiesInt = new int[numUncertainties];
		for (int j = 0; j < numUncertainties; j++) {
			uncertaintiesInt[j] = (int) uncertainties[j];
		}
		
		// Make first guess at peak locations using cross product with
		// zero area square wave.
		for (int chan = firstSearchChannel; chan < hiChannel; chan++) {
			// calc new sqwav_wid when chan is multiple of interval
			if (chan == multInterval) {
				squareWaveWidth = getSquareWaveWidth(wx, chan);
				multInterval += PS_UPDATE_INTERVAL;
			}
			
			// Calculate the cross product of square wave and count uncertainties
			// at "chan".
			// 
			// NOTE - GAUSS VII only used integer part of sigcount
			
			// from "chan" to "chan + sqwav_wid - 1"
			int squareWaveY = -1;
			int crossProduct = 0;
			int pointer = chan - spectrum.getFirstChannel();
			int top = pointer + squareWaveWidth;
			for (; pointer < top; pointer++) {
				crossProduct += squareWaveY * uncertaintiesInt[pointer];
			}
			
			// from "chan + sqwav_wid" to "chan + (sqwav_wid * 2) - 1"
			squareWaveY = 2;
			top += squareWaveWidth;
			for (; pointer < top; pointer++) {
				crossProduct += squareWaveY * uncertaintiesInt[pointer];
			}
			
			// from "chan + (sqwav_wid * 2)" to "chan + (sqwav_wid * 3) - 1"
			squareWaveY = -1;
			top += squareWaveWidth;
			for (; pointer < top; pointer++) {
				crossProduct += squareWaveY * uncertaintiesInt[pointer];
			}
			
			// store cross product for this channel
			pointer = chan - spectrum.getFirstChannel();
			crossProducts[pointer] = crossProduct;
			
		} // end for (chan = firstSearchChannel...
		PeakSearchResults results = new PeakSearchResults();
		results.setCrossProducts(crossProducts);
		
		// review cross products to find peaks
		
		TreeSet<Integer> rawPeakCentroids = new TreeSet<Integer>();
		
		int passCount = 0;
		int chan = firstSearchChannel + 2;
		i = chan - spectrum.getFirstChannel();
		for (; chan < hiChannel; i++, chan++) {
			// calculate peak width
			double peakWidth = getPeakWidth(wx, chan);
			squareWaveWidth = getSquareWaveWidth(wx, chan);
			
			if (peakWidth < PS_MAX_PEAKWIDTH) {
				// use old algorithm for marking
				
			    // If previous cross product was greater than the threshold,
			    // and this cross product is decreasing,
			    // and previous two cross products were the same or increasing,
			    // then record peak at 'previous chan + 3/2 of wave width'
			    // because this is indexing from the left end of the square wave.
				
				if ((crossProducts[i-1] > threshold) &&
					(crossProducts[i-1] > crossProducts[i]) &&
					(crossProducts[i-1] >= crossProducts[i-2])) {
					
					int foundPeak = chan - 1 + (int) (1.5 * squareWaveWidth);
					markRawPeak(rawPeakCentroids, foundPeak, peakWidth);
					
					passCount = 0;
				}
			} else {
				// use new algorithm for wider peaks
				
				if (crossProducts[i] >= threshold) {
					passCount++;
				} else {
					// cross product below threshold
					
					if (passCount > 0) {
						// want to mark half-way back plus 3/2 of wave width
						int foundPeak = (int) (chan -
								(.5 * passCount) + (1.5 * squareWaveWidth));
						markRawPeak(rawPeakCentroids, foundPeak, peakWidth);
						
						passCount = 0;
					}
				}
			}
		} // end for loop
		
		// fine-tune peak locations using linear least-square fit
		
		for (Iterator<Integer> it = rawPeakCentroids.iterator();
			 it.hasNext(); ) {
			Integer peak = it.next();
			int rawChannel = peak.intValue();
			double peakWidth = getPeakWidth(wx, rawChannel);
			SearchPeak newPeak = fitPeak(rawChannel, peakWidth, spectrum);			
			results.addPeak(newPeak);
		}

		return results;
	}
	
	// private methods

	/*
	 * getPeakWidth		routine to calculate the peak width
	 *                  of a peak at a given channel.
	 */
	private static double getPeakWidth(WidthEquation wx,
			double channel) {
		
		double answer = PS_MIN_PEAKWIDTH;
		
		try {
			answer = wx.getPeakwidth(channel);
		} catch (Exception e) {
			// use default answer
		}
		
		return answer;
	}
	
	/*
	 * fitPeak - private routine to fine-tune a peak centroid
	 *
	 *
	 * First, determine the peak width.
	 *
	 * Then take the log of the count in each channel of the peak. This
	 * takes data that is expected to be gaussian shaped and produces
	 * data that is expected to be shaped like a parabola.
	 *
	 * Then fit a parabola to the "logged" data.
	 *
	 * Finally, find the parabola's maximum which coincides with the centroid
	 * of the gaussian shape.
	 *
	 * The reference:
	 *   K. Debertin & R. G. Helmer, Gamma- and X-Ray Spectrometry with
	 *   Semiconductor Detectors, (Amsterdam: Elsevier Science B.V., 1988),
	 *   pg. 175. 
	 * says that the centroid can be determined to within .1 channels using
	 * this fine-tuning if the statistical quality of the data allows.
	 *
	 */
	private static SearchPeak fitPeak(int centroid, double peakWidth,
			Spectrum spectrum) {
		
		// If peak is too close to one of the spectrum's ends,
		// it cannot be fitted. */
		if ((centroid < spectrum.getFirstChannel() + PS_MAX_PEAKWIDTH) ||
			(centroid > spectrum.getLastChannel() - PS_MAX_PEAKWIDTH)) {
			SearchPeak peak = new SearchPeak(centroid);
			return peak;
		}
		
		// establish peak boundaries
		
		int pcw = (int) peakWidth;
		pcw = Math.min(PS_MAX_FITWIDTH_ODD, pcw);
		int hpcw = pcw / 2;
		pcw = (hpcw * 2) + 1; /* force to be odd */

		int low = centroid - hpcw;
		int high = centroid + hpcw + 1;

		// estimate average background
		
		int[] counts = spectrum.getCounts();
		int firstChannel = spectrum.getFirstChannel();
		int preAverageBack = 0;
		int top = low - 1 - firstChannel;
		for (int i = low - 5 - firstChannel; i <= top; i++) {
			preAverageBack += counts[i];
		}
		preAverageBack = preAverageBack / 5;

		int postAverageBack = 0;
		top = high + 5 - firstChannel;
		for (int i = high + 1 - firstChannel; i <= top; i++) {
			postAverageBack += counts[i];
		}
		postAverageBack = postAverageBack / 5;
		
		int averageBack = Math.min(preAverageBack, postAverageBack);
				
		// Using the Gaussian shaped peak data, construct parabolic shaped
		// peak data by taking the natural log of the number of counts
		// in each channel.
		// Also, accumulate the net area & average background
		
		ChannelRange region = new ChannelRange(centroid - hpcw, centroid + hpcw);
		double netArea = 0;
		double background = 0;
		double[] x = new double[pcw];
		double[] y = new double[pcw];
		for (int i = 0, j = low - firstChannel; i < pcw; i++, j++) {
			x[i] = i;
			double netCounts = Math.max(1, counts[j] - averageBack);
			y[i] = Math.log(netCounts);
			netArea += counts[j] - averageBack;
			background += averageBack;
		}
		boolean useRefinement = false;
		
		// To fit the parabolic shaped peak data, follow Bevington's method for
		// solving a matrix equation for a least-squares fit to a polynomial.
		// (P.R. Bevington, Data Reduction and Error Analysis for the Physical
		//  Sciences (New York: McGraw-Hill, 2003), p. 116-123, 238-244.)
		//
		// Define the row matrix "Beta" to have as its components the 0th, 1st, and
		// 2nd moments of y[i] with respect to x[i].
		//
		//     Beta = (B1 B2 B3)
		//
		// Define the row matrix "a" to have as its components the coefficients of
		// the polynomial describing the fitted parabola y = a1 + a2*x + a3*x**2.
		//
		//     a = (a1 a2 a3)
		//
		// Define the symmetric matrix Alpha to have as its components the 0th
		// through 4th moments of x[i].
		//
		//             (sumx0 sumx1 sumx2)
		//     Alpha = (sumx1 sumx2 sumx3)
		//             (sumx2 sumx3 sumx4)
		//
		// Then the matrix equation is
		//
		//     Beta = a * Alpha    (see equation B.6 in Bevington)
		//
		// The formula for a[i] is shown in equation B.33, and an example of
		// computing 3 x 3 determinants is in equations B.20 and B.21.
		//
		//
		// The following reference shows the solution for a[i] in terms of
		// Cramer's rule: (same solution in different notation)
		//    D.C. Lay, Linear Algebra and Its Applications, 2nd Ed.
		//    (Reading, MA: Addison-Wesley, 1999), p. 195-199.

		// calculate components for the matrices Beta and Alpha
		
		double B1 = 0, B2 = 0, B3 = 0;
		double sumx0 = 0, sumx1 = 0, sumx2 = 0, sumx3 = 0, sumx4 = 0;
		
		for (int i = 0; i < pcw; i++) {
			B1 += y[i];
			B2 += y[i] * x[i];
			B3 += y[i] * x[i] * x[i];
			sumx0 += 1;
			sumx1 += x[i];
			sumx2 += x[i] * x[i];
			sumx3 += x[i] * x[i] * x[i];
			sumx4 += x[i] * x[i] * x[i] * x[i];
		}
		
		// Calculate the product of the determinate of Alpha times a3 by
		// calculating the determinate on the right in the following equation.
		//
		//                |sumx0 sumx1 B1|
		// |Alpha| * a3 = |sumx1 sumx2 B2|  (see B.33 in Bevington)
		//                |sumx2 sumx3 B3|
		
		// determinant of |Alpha| times parabola's quadratic coefficient
		double detAlpha_a3 = (sumx0 * sumx2 * B3);
		detAlpha_a3 += - (sumx0 * sumx3 * B2);
		detAlpha_a3 += - (sumx1 * sumx1 * B3);
		detAlpha_a3 += (sumx1 * B2 * sumx2);
		detAlpha_a3 += (B1 * sumx1 * sumx3);
		detAlpha_a3 += - (B1 * sumx2 * sumx2);

		double newCentroid;
		if (detAlpha_a3 == 0.0) {
			// cannot improve the centroid location
			newCentroid = centroid;
		} else {
			// Calculate the product of the determinate of Alpha times a2 by
			// calculating the determinate on the right in the following equation.
			//
			//                |sumx0 B1 sumx2|
			// |Alpha| * a2 = |sumx1 B2 sumx3|  (see B.33 in Bevington)
			//                |sumx2 B3 sumx4|
			
			// determinant of |Alpha| times parabola's linear coefficient
			double detAlpha_a2 = (sumx0 * B2 * sumx4);
			detAlpha_a2 += - (sumx0 * sumx3 * B3);
			detAlpha_a2 += - (B1 * sumx1 * sumx4);
			detAlpha_a2 += (B1 * sumx3 * sumx2);
			detAlpha_a2 += (sumx2 * sumx1 * B3);
			detAlpha_a2 += - (sumx2 * B2 * sumx2);
			
			// Find the location of the maximum of the fitted parabola which
			// is also the location of the Gaussian centroid. The maximum of
			// the parabola will be where the slope is zero.
			//
			// The formula for the slope is y' = a2 + (2 * a3 * x).
			//
			// Solving for x where y' is zero yields: x = -a2 / (2 * a3)
			//
			// The values "|Alpha| * a3" and "|Alpha| * a2" have already
			// been calculated. Taking a ratio of these values as follows:
			//
			//     x = (-|Alpha| * a2)/(2 * |Alpha| * a3)
			//
			// is a convenient way to calculate x because |Alpha| cancels out
			// from the numerator and denominator.
			
			newCentroid = (- detAlpha_a2 / (2.0 * detAlpha_a3)) + low;

			// If the new centroid is within a half peakwidth of the original,
			// use the new centroid.
			//
			// Otherwise, use the original centroid.
						
			double diff = Math.abs(centroid - newCentroid);
			if (diff <= hpcw) {
				useRefinement = true;
			}
		}
		
		SearchPeak peak = new SearchPeak(centroid, region, netArea,
				background, newCentroid, useRefinement);

		return(peak);
	}
	
	/*
	 * markPeak - add a peak to the integer list if it is not closer than
	 *            a peakwidth to the previous peak
	 */
	private static void markRawPeak(TreeSet<Integer> rawPeakCentroids,
			int newPeak, double peakWidth) {
				
		Integer previousPeak = null;
		if (0 < rawPeakCentroids.size()) {
			previousPeak = rawPeakCentroids.last();
		}
		
		if (null != previousPeak) {
			int previousPeakInt = previousPeak.intValue();
			double distance = previousPeakInt + peakWidth;
			if (distance >= newPeak) {
				return;
			}
		}
		
		rawPeakCentroids.add(new Integer(newPeak));
	}

	/*
	 * getSquareWaveWidth - calculate width of the square wave
	 */
	private static int getSquareWaveWidth(WidthEquation wx, double channel) {
		
		int sqwav_wid = PS_MIN_SQWAV;
		
		double peakWidthDouble = getPeakWidth(wx, channel);
		if (0 < peakWidthDouble) {
			sqwav_wid = (int) peakWidthDouble;
			sqwav_wid = ((sqwav_wid/2) * 2) + 1;	/* ensure it is odd integer */
			sqwav_wid = Math.max(PS_MIN_SQWAV, sqwav_wid);
		}
	
		return sqwav_wid;
	}
	
} // end PeakSearching
