package org.astron.lofarsim;

import org.astron.basesim.GraphWorkHolderImage;
import org.astron.basesim.*;
import org.astron.basesim.def.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2001
 * Company:      ASTRON
 * @author Robbert Dam
 * @version 1.0
 */

public class WHMakeMS extends GraphWorkHolderImage {

  public WHMakeMS() {
    super();
  }

  public String getClassName() { return "Make MS"; }
  public GraphDataHolder buildDataHolder(boolean input) {
    if (input) {
      return new DHCorr();
    } else {
      return new DHEmpty();
    }
  }

}