// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "asset_object.h"
#include "viewmodel/helpers/list_model.h"
#include <memory>

class AssetsList : public ListModel<std::shared_ptr<AssetObject>>
{
    Q_OBJECT
public:
    AssetsList() = default;
    ~AssetsList() override = default;

    enum class Roles
    {
        Search = Qt::UserRole + 1,
        RId,
        RName,
        RAmount,
        RInTxCnt,
        ROutTxCnt,
    };

    Q_ENUM(Roles)

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
};
