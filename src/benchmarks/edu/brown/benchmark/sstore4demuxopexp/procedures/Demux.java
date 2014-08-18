/* This file is part of VoltDB.
 * Copyright (C) 2008-2012 VoltDB Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

//
// Accepts a vote, enforcing business logic: make sure the vote is for a valid
// contestant and that the voterdemosstore (phone number of the caller) is not above the
// number of allowed votes.
//

package edu.brown.benchmark.sstore4demuxopexp.procedures;

import org.voltdb.ProcInfo;
import org.voltdb.SQLStmt;
import org.voltdb.StmtInfo;
import org.voltdb.VoltProcedure;
import org.voltdb.VoltTable;
import org.voltdb.types.TimestampType;

import edu.brown.benchmark.sstore4demuxopexp.SStore4DemuxOpExpConstants;

@ProcInfo (
	partitionInfo = "s1.part_id:0",
	partitionNum = 0,
	singlePartition = true
)
public class Demux extends VoltProcedure {
    HashMap<Integer, SQLStmt> procMap = new HashMap<Integer, SQLStmt>();

    protected void toSetTriggerTableName() {
        addTriggerTable("s1");
    }

    public final SQLStmt pullFromS1 = new SQLStmt(
        "SELECT vote_id, part_id FROM s1;"
    );

    public final SQLStmt ins11Stmt = new SQLStmt(
	    "INSERT INTO s11 (vote_id, part_id) VALUES (?, ?);"
    );

    public final SQLStmt ins12Stmt = new SQLStmt(
    	"INSERT INTO s12 (vote_id, part_id) VALUES (?, ?);"
    );
 
    public final SQLStmt ins13Stmt = new SQLStmt(
        "INSERT INTO s13 (vote_id, part_id) VALUES (?, ?);"
    );
 
    public final SQLStmt ins14Stmt = new SQLStmt(
        "INSERT INTO s14 (vote_id, part_id) VALUES (?, ?);"
    );

    public final SQLStmt ins15Stmt = new SQLStmt(
        "INSERT INTO s15 (vote_id, part_id) VALUES (?, ?);"
    );

    public final SQLStmt ins16Stmt = new SQLStmt(
        "INSERT INTO s16 (vote_id, part_id) VALUES (?, ?);"
    );

    public final SQLStmt ins17Stmt = new SQLStmt(
        "INSERT INTO s17 (vote_id, part_id) VALUES (?, ?);"
    );

    public final SQLStmt clearS1 = new SQLStmt(
    	"DELETE FROM s1;"
    );

    public long run(int part_id) {
        procMap.put(0, ins11Stmt);
        procMap.put(1, ins12Stmt);
        procMap.put(2, ins13Stmt);
        procMap.put(3, ins14Stmt);
        procMap.put(4, ins15Stmt);
        procMap.put(5, ins16Stmt);
        procMap.put(6, ins17Stmt);

	voltQueueSQL(pullFromS1);
	VoltTable s1Data[] = voltExecuteSQLForceSinglePartition();

	for (int i=0; i < s1Data[0].getRowCount(); i++) {
	    int vote_id = (int)(s1Data[0].fetchRow(i).getLong(0));
	    voltQueueSQL(procMap.get(vote_id % 7), vote_id, part_id);
	}
	voltExecuteSQLForceSinglePartition();

        voltQueueSQL(clearS1);
        voltExecuteSQLForceSinglePartition();
				
        // Set the return value to 0: successful vote
        return SStore4DemuxOpExpConstants.VOTE_SUCCESSFUL;
    }
}
