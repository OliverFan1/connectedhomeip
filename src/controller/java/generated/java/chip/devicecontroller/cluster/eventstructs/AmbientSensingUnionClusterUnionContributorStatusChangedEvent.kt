/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package chip.devicecontroller.cluster.eventstructs

import chip.devicecontroller.cluster.*
import matter.tlv.AnonymousTag
import matter.tlv.ContextSpecificTag
import matter.tlv.Tag
import matter.tlv.TlvReader
import matter.tlv.TlvWriter

class AmbientSensingUnionClusterUnionContributorStatusChangedEvent(
  val previousContributorStatus:
    List<chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct>,
  val currentContributorStatus:
    List<chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct>,
) {
  override fun toString(): String = buildString {
    append("AmbientSensingUnionClusterUnionContributorStatusChangedEvent {\n")
    append("\tpreviousContributorStatus : $previousContributorStatus\n")
    append("\tcurrentContributorStatus : $currentContributorStatus\n")
    append("}\n")
  }

  fun toTlv(tlvTag: Tag, tlvWriter: TlvWriter) {
    tlvWriter.apply {
      startStructure(tlvTag)
      startArray(ContextSpecificTag(TAG_PREVIOUS_CONTRIBUTOR_STATUS))
      for (item in previousContributorStatus.iterator()) {
        item.toTlv(AnonymousTag, this)
      }
      endArray()
      startArray(ContextSpecificTag(TAG_CURRENT_CONTRIBUTOR_STATUS))
      for (item in currentContributorStatus.iterator()) {
        item.toTlv(AnonymousTag, this)
      }
      endArray()
      endStructure()
    }
  }

  companion object {
    private const val TAG_PREVIOUS_CONTRIBUTOR_STATUS = 0
    private const val TAG_CURRENT_CONTRIBUTOR_STATUS = 1

    fun fromTlv(
      tlvTag: Tag,
      tlvReader: TlvReader,
    ): AmbientSensingUnionClusterUnionContributorStatusChangedEvent {
      tlvReader.enterStructure(tlvTag)
      val previousContributorStatus =
        buildList<
          chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct
        > {
          tlvReader.enterArray(ContextSpecificTag(TAG_PREVIOUS_CONTRIBUTOR_STATUS))
          while (!tlvReader.isEndOfContainer()) {
            this.add(
              chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct
                .fromTlv(AnonymousTag, tlvReader)
            )
          }
          tlvReader.exitContainer()
        }
      val currentContributorStatus =
        buildList<
          chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct
        > {
          tlvReader.enterArray(ContextSpecificTag(TAG_CURRENT_CONTRIBUTOR_STATUS))
          while (!tlvReader.isEndOfContainer()) {
            this.add(
              chip.devicecontroller.cluster.structs.AmbientSensingUnionClusterUnionContributorStruct
                .fromTlv(AnonymousTag, tlvReader)
            )
          }
          tlvReader.exitContainer()
        }

      tlvReader.exitContainer()

      return AmbientSensingUnionClusterUnionContributorStatusChangedEvent(
        previousContributorStatus,
        currentContributorStatus,
      )
    }
  }
}
