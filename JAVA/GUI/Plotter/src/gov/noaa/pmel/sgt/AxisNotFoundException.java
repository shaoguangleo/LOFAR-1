/*
 * $Id$
 *
 * This software is provided by NOAA for full, free and open release.  It is
 * understood by the recipient/user that NOAA assumes no liability for any
 * errors contained in the code.  Although this software is released without
 * conditions or restrictions in its use, it is expected that appropriate
 * credit be given to its author and to the National Oceanic and Atmospheric
 * Administration should the software be included by the recipient as an
 * element in other product development.
 */

package  gov.noaa.pmel.sgt;

/**
 * Axis was not found during operation.
 *
 * @author Donald Denbo
 * @version $Revision$, $Date$
 */
public class AxisNotFoundException extends SGException {
  public AxisNotFoundException() {
    super();
}
  public AxisNotFoundException(String s) {
    super(s);
  }
}
